#if !defined(DEBUG_LINK_H)
#define DEBUG_LINK_H 1

bool debug_link_check_active();
bool debug_link_update();
void debug_link_status(char *str, int len);
int debug_link_read(void *buffer, int maxlen);
int debug_link_write(const void *data, int len);

#endif // DEBUG_H
