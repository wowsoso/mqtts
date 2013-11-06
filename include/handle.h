#include "mqtts.h"

typedef int (*handler_t)(mqtts_t*, int, fixed_header_t*, void*, void*, int, int);

/* int handle_connack(mqtts_t* mqtts, int sock, int return_code); */
int handle_connect(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step);
int handle_unknown(mqtts_t* mqtts, int sock, fixed_header_t* fixed_header, void* variable_header, void* payload, int fixed_header_length, uint8_t step);
handler_t get_handler(uint8_t command);
int handle(mqtts_t* mqtts, int sock);
int handle_connack(mqtts_t* mqtts, int sock);
int handle_puback(mqtts_t* mqtts, int sock);
