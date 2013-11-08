#include "mqtts.h"

typedef int (*handler_t)(mqtts_t*, int, fixed_header_t*, void*, void*, int, int);

handler_t get_handler(uint8_t command);
int handle(mqtts_t* mqtts, int sock);
