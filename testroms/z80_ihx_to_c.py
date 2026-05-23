#!/usr/bin/env python3
import sys
from pathlib import Path


def parse_ihx(path: Path):
    data = {}
    upper = 0
    max_addr = 0
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if not line.startswith(':'):
            raise SystemExit(f"{path}:{lineno}: not an Intel HEX record")
        raw = bytes.fromhex(line[1:])
        count = raw[0]
        addr = (raw[1] << 8) | raw[2]
        rectype = raw[3]
        payload = raw[4:4 + count]
        if rectype == 0x00:
            base = upper + addr
            for i, b in enumerate(payload):
                data[base + i] = b
            max_addr = max(max_addr, base + count)
        elif rectype == 0x01:
            break
        elif rectype == 0x04:
            upper = ((payload[0] << 8) | payload[1]) << 16
        elif rectype == 0x02:
            upper = ((payload[0] << 8) | payload[1]) << 4
    return bytes(data.get(i, 0) for i in range(max_addr))


def write_header(blob: bytes, out: Path, symbol: str):
    with out.open('w') as f:
        f.write('#if !defined(Z80_ICS_DRIVER_DATA_H)\n')
        f.write('#define Z80_ICS_DRIVER_DATA_H 1\n\n')
        f.write(f'static const unsigned char {symbol}[] = {{\n')
        for i in range(0, len(blob), 12):
            chunk = ', '.join(f'0x{b:02x}' for b in blob[i:i + 12])
            f.write(f'    {chunk},\n')
        f.write('};\n')
        f.write(f'static const unsigned int {symbol}_size = {len(blob)};\n\n')
        f.write('#endif\n')


def main(argv):
    if len(argv) != 4:
        raise SystemExit('usage: z80_ihx_to_c.py input.ihx output.h symbol')
    blob = parse_ihx(Path(argv[1]))
    write_header(blob, Path(argv[2]), argv[3])


if __name__ == '__main__':
    main(sys.argv)
