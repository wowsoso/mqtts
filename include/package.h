struct mqtts_packet_s{
    uint8_t command;
    uint8_t have_remaining;
    uint8_t remaining_count;
    uint32_t remaining_mult;
    uint32_t remaining_length;
    uint32_t packet_length;
    uint32_t to_process;
    uint32_t pos;
    uint8_t *payload;
    struct _mosquitto_packet *next;
};

char* unpack_string(char* string);
char* pack_string(char* string);
