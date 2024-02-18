#include "presentation.h"

/*
    Presentation functions
 */
void format_traffic_lights(u_int8_t n, char *binstr) {
    memset(binstr, 0, 9);
    for (int i = 7; i >= 0; i--) {
        int k = n >> i;
        if (k & 1)
            binstr[7 - i] = '1';
        else
            binstr[7 - i] = '0';
    }

    binstr[0] = (binstr[0] == '1' ? 'R' : '.');
    binstr[1] = (binstr[1] == '1' ? 'Y' : '.');
    binstr[2] = (binstr[2] == '1' ? 'G' : '.');
    binstr[3] = (binstr[3] == '1' ? '<' : '.');
    binstr[4] = (binstr[4] == '1' ? '>' : '.');
    binstr[5] = (binstr[5] == '1' ? '?' : '.');
    binstr[6] = (binstr[6] == '1' ? '?' : '.');
    binstr[7] = (binstr[7] == '1' ? 'B' : '.');

}


char* blinkOn(char a, char b) {
    return ((b != '.') && (a != '.'))?ANSI_BLINK_ON:"";
}

char* blinkOff(char a, char b) {
    return ((b != '.') && (a != '.'))?ANSI_BLINK_OFF:"";
}

char *colorize_traffic_lights(char *binstr, char *colorized) {
    if (colorized == NULL) return "--------";

    sprintf(colorized, "%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s..%s%s%c%s",
            blinkOn(binstr[0], binstr[7]),
            (binstr[0] != '.')?ANSI_COLOR_RED:"",
            (binstr[0] != '.')?'R':'.',
            (binstr[0] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[0], binstr[7]),

            blinkOn(binstr[1], binstr[7]),
            (binstr[1] != '.')?ANSI_COLOR_YELLOW:"",
            (binstr[1] != '.')?'Y':'.',
            (binstr[1] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[1], binstr[7]),

            blinkOn(binstr[2], binstr[7]),
            (binstr[2] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[2] != '.')?'G':'.',
            (binstr[2] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[2], binstr[7]),

            blinkOn(binstr[3], binstr[7]),
            (binstr[3] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[3] != '.')?'<':'.',
            (binstr[3] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[3], binstr[7]),

            blinkOn(binstr[4], binstr[7]),
            (binstr[4] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[4] != '.')?'>':'.',
            (binstr[4] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[4], binstr[7]),

            blinkOn(binstr[5], binstr[7]),
            blinkOff(binstr[5], binstr[7]),

            (binstr[7] != '.')?ANSI_COLOR_CYAN:"",
            (binstr[7] != '.')?'B':'.',
            (binstr[7] != '.')?ANSI_COLOR_RESET:""

    );
    return colorized;
}


