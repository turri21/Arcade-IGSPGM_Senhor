#include "sim_command.h"
#include <fstream>
#include <sstream>
#include <getopt.h>
#include <cstring>
#include <cstdio>
#include <cctype>

static std::string Lowercase(std::string value)
{
    for (char &ch : value)
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return value;
}

static bool ParseDipSwitchState(const std::string &text, bool &enabled)
{
    std::string value = Lowercase(text);
    if (value == "on" || value == "1" || value == "true" || value == "enable" || value == "enabled")
    {
        enabled = true;
        return true;
    }
    if (value == "off" || value == "0" || value == "false" || value == "disable" || value == "disabled")
    {
        enabled = false;
        return true;
    }
    return false;
}

static bool MakeDipSwitchCommand(const std::string &switchText, const std::string &stateText, uint64_t &encoded)
{
    uint64_t switchNumber = 0;
    try
    {
        switchNumber = std::stoull(switchText);
    }
    catch (...)
    {
        return false;
    }

    bool enabled = false;
    if (switchNumber < 1 || switchNumber > 8 || !ParseDipSwitchState(stateText, enabled))
        return false;

    encoded = (switchNumber - 1) | (enabled ? 0x100 : 0);
    return true;
}

static bool ParseDipSwitchArgument(const std::string &arg, uint64_t &encoded)
{
    size_t sep = arg.find_first_of("=:/,");
    if (sep == std::string::npos)
        return false;
    return MakeDipSwitchCommand(arg.substr(0, sep), arg.substr(sep + 1), encoded);
}

void CommandQueue::Add(const Command &cmd)
{
    mCommands.push(cmd);
    if (cmd.mType != CommandType::EXIT)
        mBatchMode = true;
}

bool CommandQueue::ParseArguments(int argc, char **argv, std::string &gameName)
{
    static struct option sLongOptions[] = {{"load-state", required_argument, 0, 'l'},
                                           {"save-state", required_argument, 0, 's'},
                                           {"run-cycles", required_argument, 0, 'c'},
                                           {"run-frames", required_argument, 0, 'f'},
                                           {"screenshot", required_argument, 0, 'p'},
                                           {"trace-start", required_argument, 0, 't'},
                                           {"trace-stop", no_argument, 0, 'T'},
                                           {"script", required_argument, 0, 'x'},
                                           {"load-game", required_argument, 0, 'g'},
                                           {"load-mra", required_argument, 0, 'm'},
                                           {"reset", required_argument, 0, 'r'},
                                           {"dipswitch", required_argument, 0, 'D'},
                                           {"headless", no_argument, 0, 'h'},
                                           {"verbose", no_argument, 0, 'v'},
                                           {"help", no_argument, 0, '?'},
                                           {0, 0, 0, 0}};

    int optionIndex = 0;
    int c;

    // Reset getopt
    optind = 1;

    while ((c = getopt_long(argc, argv, "l:s:c:f:p:t:Tx:g:m:r:D:hv?", sLongOptions, &optionIndex)) != -1)
    {
        switch (c)
        {
        case 'l':
            Add(Command(CommandType::LOAD_STATE, optarg));
            if (mVerbose)
                printf("Command: Load state from %s\n", optarg);
            break;

        case 's':
            Add(Command(CommandType::SAVE_STATE, optarg));
            if (mVerbose)
                printf("Command: Save state to %s\n", optarg);
            break;

        case 'c':
        {
            uint64_t cycles = std::stoull(optarg);
            Add(Command(CommandType::RUN_CYCLES, cycles));
            if (mVerbose)
                printf("Command: Run for %llu cycles\n", cycles);
        }
        break;

        case 'f':
        {
            uint64_t frames = std::stoull(optarg);
            Add(Command(CommandType::RUN_FRAMES, frames));
            if (mVerbose)
                printf("Command: Run for %llu frames\n", frames);
        }
        break;

        case 'p':
            Add(Command(CommandType::SCREENSHOT, optarg));
            if (mVerbose)
                printf("Command: Save screenshot to %s\n", optarg);
            break;

        case 't':
            Add(Command(CommandType::TRACE_START, optarg));
            if (mVerbose)
                printf("Command: Start trace to %s\n", optarg);
            break;

        case 'T':
            Add(Command(CommandType::TRACE_STOP));
            if (mVerbose)
                printf("Command: Stop trace\n");
            break;

        case 'x':
            if (!ParseScriptFile(optarg))
            {
                printf("Error: Failed to parse script file: %s\n", optarg);
                return false;
            }
            break;

        case 'g':
            Add(Command(CommandType::LOAD_GAME, optarg));
            if (mVerbose)
                printf("Command: Load game %s\n", optarg);
            break;

        case 'm':
            Add(Command(CommandType::LOAD_MRA, optarg));
            if (mVerbose)
                printf("Command: Load MRA %s\n", optarg);
            break;

        case 'r':
        {
            uint64_t cycles = std::stoull(optarg);
            Add(Command(CommandType::RESET, cycles));
            if (mVerbose)
                printf("Command: Reset for %llu cycles\n", cycles);
        }
        break;

        case 'D':
        {
            uint64_t value = 0;
            if (!ParseDipSwitchArgument(optarg, value))
            {
                printf("Error: --dipswitch expects <1-8>=<on|off>\n");
                return false;
            }
            Add(Command(CommandType::SET_DIPSWITCH, value));
            if (mVerbose)
                printf("Command: Turn dipswitch %llu %s\n", (value & 7) + 1, (value & 0x100) ? "on" : "off");
        }
        break;

        case 'h':
            mHeadless = true;
            if (mVerbose)
                printf("Running in headless mode\n");
            break;

        case 'v':
            mVerbose = true;
            printf("Verbose mode enabled\n");
            break;

        case '?':
            PrintUsage(argv[0]);
            exit(0);
            break;

        default:
            printf("Unknown option: %c\n", c);
            PrintUsage(argv[0]);
            return false;
        }
    }

    // Get game name (positional argument)
    if (optind < argc)
    {
        gameName = argv[optind];
    }
    else if (!mBatchMode)
    {
        gameName = "finalb"; // Default game
    }

    // Add implicit exit only for headless mode
    if (mHeadless)
    {
        Add(Command(CommandType::EXIT));
    }

    return true;
}

bool CommandQueue::ParseScriptFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        printf("Error: Cannot open script file: %s\n", filename.c_str());
        return false;
    }

    if (mVerbose)
        printf("Parsing script file: %s\n", filename.c_str());

    std::string line;
    int lineNum = 0;

    while (std::getline(file, line))
    {
        lineNum++;

        // Skip empty lines and comments
        size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string::npos || line[firstNonSpace] == '#')
            continue;

        if (!ParseScriptLine(line))
        {
            printf("Error in script file %s at line %d: %s\n", filename.c_str(), lineNum, line.c_str());
            return false;
        }
    }

    return true;
}

bool CommandQueue::ParseScriptLine(const std::string &line)
{
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "load-state" || command == "load_state")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
            return false;
        Add(Command(CommandType::LOAD_STATE, filename));
        if (mVerbose)
            printf("Script: Load state from %s\n", filename.c_str());
    }
    else if (command == "save-state" || command == "save_state")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
            return false;
        Add(Command(CommandType::SAVE_STATE, filename));
        if (mVerbose)
            printf("Script: Save state to %s\n", filename.c_str());
    }
    else if (command == "run-cycles" || command == "run_cycles")
    {
        uint64_t cycles;
        iss >> cycles;
        if (iss.fail())
            return false;
        Add(Command(CommandType::RUN_CYCLES, cycles));
        if (mVerbose)
            printf("Script: Run for %llu cycles\n", cycles);
    }
    else if (command == "run-frames" || command == "run_frames")
    {
        uint64_t frames;
        iss >> frames;
        if (iss.fail())
            return false;
        Add(Command(CommandType::RUN_FRAMES, frames));
        if (mVerbose)
            printf("Script: Run for %llu frames\n", frames);
    }
    else if (command == "screenshot")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
            return false;
        Add(Command(CommandType::SCREENSHOT, filename));
        if (mVerbose)
            printf("Script: Save screenshot to %s\n", filename.c_str());
    }
    else if (command == "trace-start" || command == "trace_start")
    {
        std::string filename;
        iss >> filename;
        if (filename.empty())
            return false;
        Add(Command(CommandType::TRACE_START, filename));
        if (mVerbose)
            printf("Script: Start trace to %s\n", filename.c_str());
    }
    else if (command == "trace-stop" || command == "trace_stop")
    {
        Add(Command(CommandType::TRACE_STOP));
        if (mVerbose)
            printf("Script: Stop trace\n");
    }
    else if (command == "load-game" || command == "load_game")
    {
        std::string gameName;
        iss >> gameName;
        if (gameName.empty())
            return false;
        Add(Command(CommandType::LOAD_GAME, gameName));
        if (mVerbose)
            printf("Script: Load game %s\n", gameName.c_str());
    }
    else if (command == "load-mra" || command == "load_mra")
    {
        std::string mraPath;
        iss >> mraPath;
        if (mraPath.empty())
            return false;
        Add(Command(CommandType::LOAD_MRA, mraPath));
        if (mVerbose)
            printf("Script: Load MRA %s\n", mraPath.c_str());
    }
    else if (command == "reset")
    {
        uint64_t cycles;
        iss >> cycles;
        if (iss.fail())
            return false;
        Add(Command(CommandType::RESET, cycles));
        if (mVerbose)
            printf("Script: Reset for %llu cycles\n", cycles);
    }
    else if (command == "dipswitch" || command == "dip-switch" || command == "dip_switch")
    {
        std::string switchText;
        std::string stateText;
        iss >> switchText >> stateText;
        if (switchText.empty() || stateText.empty())
            return false;
        uint64_t value = 0;
        if (!MakeDipSwitchCommand(switchText, stateText, value))
            return false;
        Add(Command(CommandType::SET_DIPSWITCH, value));
        if (mVerbose)
            printf("Script: Turn dipswitch %llu %s\n", (value & 7) + 1, (value & 0x100) ? "on" : "off");
    }
    else if (command == "wait" || command == "delay")
    {
        uint64_t ms;
        iss >> ms;
        if (iss.fail())
            return false;
        // Convert milliseconds to cycles (assuming 12MHz)
        uint64_t cycles = ms * 12000;
        Add(Command(CommandType::RUN_CYCLES, cycles));
        if (mVerbose)
            printf("Script: Wait for %llu ms (%llu cycles)\n", ms, cycles);
    }
    else
    {
        printf("Unknown command: %s\n", command.c_str());
        return false;
    }

    return true;
}

void CommandQueue::PrintUsage(const char *programName)
{
    printf("Usage: %s [options] [game_name]\n", programName);
    printf("\nOptions:\n");
    printf("  --load-state <file>    Load savestate from file\n");
    printf("  --save-state <file>    Save current state to file\n");
    printf("  --run-cycles <N>       Run simulation for N cycles\n");
    printf("  --run-frames <N>       Run simulation for N frames\n");
    printf("  --screenshot <file>    Save screenshot to file\n");
    printf("  --trace-start <file>   Start FST trace to file\n");
    printf("  --trace-stop           Stop FST trace\n");
    printf("  --script <file>        Execute commands from script file\n");
    printf("  --load-game <name>     Load game by name (e.g. finalb, cameltry)\n");
    printf("  --load-mra <file>      Load game from MRA file\n");
    printf("  --reset <cycles>       Reset for specified number of cycles\n");
    printf("  --dipswitch <1-8>=<on|off>\n");
    printf("  --headless             Run without GUI (batch mode only)\n");
    printf("  --verbose              Print command execution details\n");
    printf("  --help                 Show this help message\n");
    printf("\nScript file format:\n");
    printf("  # Comments start with #\n");
    printf("  load-game finalb\n");
    printf("  dipswitch 1 on\n");
    printf("  dipswitch 1 off\n");
    printf("  reset 100\n");
    printf("  load-state checkpoint.state\n");
    printf("  run-frames 100\n");
    printf("  trace-start debug.fst\n");
    printf("  run-frames 50\n");
    printf("  trace-stop\n");
    printf("  screenshot test.png\n");
    printf("  save-state final.state\n");
    printf("\nExample:\n");
    printf("  %s finalb --load-state test.state --run-frames 60 --screenshot out.png\n", programName);
    printf("  %s --load-game finalb --run-frames 60 --screenshot out.png\n", programName);
    printf("  %s --load-mra test.mra --headless --run-frames 100\n", programName);
    printf("  %s --script test_sequence.txt\n", programName);
}
