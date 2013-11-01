#include "mqtts.h"

typedef int (*handler_t)(mqtts_t*, int, int, int, int);

int handle_connect(mqtts_t* mqtts, int sock, int remaining_length, int fixed_header_length, uint8_t step);
int handle_unknown(mqtts_t* mqtts, int sock, int remaining_length, int fixed_header_length, uint8_t step);
handler_t get_handler(uint8_t command);
int handle(mqtts_t* mqtts, int sock);
