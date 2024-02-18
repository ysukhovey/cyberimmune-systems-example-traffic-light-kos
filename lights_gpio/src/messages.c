#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rtl/string.h>
#include <rtl/stdio.h>

#include "messages.h"

void send_diagnostic_message(TransportDescriptor *desc, u_int32_t in_code, char *in_message) {
    if (in_message == NULL) return;

    int logMessageLength = 0;
    char logMessage[traffic_light_IDiagMessage_Write_req_inMessage_message_size];
    rtl_memset(logMessage, 0, traffic_light_IDiagMessage_Write_req_inMessage_message_size);

    nk_arena_reset(desc->reqArena);

    nk_ptr_t *message = nk_arena_alloc(nk_ptr_t, desc->reqArena, &(desc->req->inMessage.message), 1);
    if (message == RTL_NULL) {
        fprintf(stderr, "[LightsGPIO   ] Error: can`t allocate memory in arena!\n");
        return;
    }

    logMessageLength = rtl_snprintf(logMessage, traffic_light_IDiagMessage_Write_req_inMessage_message_size, in_message);
    if (logMessageLength < 0) {
        fprintf(stderr, "[LightsGPIO   ] Error: length of message is negative number!\n");
        return;
    }

    nk_char_t *str = nk_arena_alloc(nk_char_t, desc->reqArena, &(message[0]), (nk_size_t) (logMessageLength + 1));
    if (str == RTL_NULL) {
        fprintf(stderr, "[LightsGPIO   ] Error: can`t allocate memory in arena!\n");
        return;
    }

    rtl_strncpy(str, logMessage, (rtl_size_t) (logMessageLength + 1));
    desc->req->inMessage.code = in_code;

    fprintf(stderr, "[LightsGPIO   ] ==> %08d: %s\n", desc->req->inMessage.code, in_message);

    if (traffic_light_IDiagMessage_Write(&desc->proxy->base, desc->req, desc->reqArena, desc->res, desc->resArena) != NK_EOK) {
        fprintf(stderr, "[LightsGPIO   ] Error: can`t send message to HardwareDiagnostic entity!\n");
        return;
    }
}
