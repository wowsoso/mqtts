#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "mqtts.h"

int handle(int sock, package_buf_t* buf)
{
    byte = buf->package[0];
    if (byte && 0xF0 == 0x01)
    {
        handle_connect(sock, buf);
    }
}

int handle_connect(int sock, package_buf_t* buf)
{
    printf ("%s\n", "connect");
    printf ("%s\n", buf->package);
}

/* ssize_t mqtts_read(int sock, void *buf, size_t count) */
/* { */
/*     return read(sock, buf, count); */
/* } */



/* static char* _strncpy(char *dest, const char *src, size_t n) */
/* { */
/*     size_t i; */

/*     for (i = 0; i < n; i++) */
/*     { */
/*         dest[i] = src[i]; */
/*     } */

/*     return dest; */
/* } */



/* void unpack_string(char* string, string_t* str) */
/* { */
/*     /\* str->length = 0; *\/ */

/*     _strncpy (&(str->length), string, 2); */
/*     strncpy (str->text, string + 2, sizeof(string) - 2); */
/*     str->length = htons (str->length); */

/*     /\* str->text = strncpy(&(str->text), string + 2, sizeof(string) - 2); */
/*     /\* str->text = "teee"; *\/ */
/* } */



/* int main(int argc, char *argv[]) */
/* { */
/*     string_t str; */
/*     /\* char s[10] = *\/ */
/*     char* s = "\x00\x06MQIsdp"; */
/*     /\* str.length = 2; *\/ */
/*     str.text = malloc ((strlen (s) - 2) * sizeof(s)); */
/*     printf ("%d\n", (strlen (s))); */
/*     unpack_string (s, &str); */


/*     /\* str->length = htons(str->length); *\/ */
/*     /\* str->text = "fuck"; *\/ */
/*     printf ("%s\n", str.text); */
/*     printf ("%d\n", str.length); */
/*     /\* printf ("%d\n", sizeof(s)); *\/ */
/*     return 0; */
/* } */
