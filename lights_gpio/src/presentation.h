#ifndef CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_PRESENTATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <rtl/string.h>
#include <rtl/stdio.h>

#define ANSI_COLOR_BLACK    "\x1b[30m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RED      "\x1b[91m"
#define ANSI_COLOR_YELLOW   "\x1b[93m"
#define ANSI_COLOR_GREEN    "\x1b[92m"
#define ANSI_COLOR_RESET    "\x1b[0m"
#define ANSI_BLINK_ON       "\x1b[5m"
#define ANSI_BLINK_OFF      "\x1b[25m"

void format_traffic_lights(u_int8_t n, char *binstr);
char* blinkOn(char a, char b);
char* blinkOff(char a, char b);
char* colorize_traffic_lights(char *binstr, char *colorized);

#define CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_PRESENTATION_H

#endif //CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_PRESENTATION_H
