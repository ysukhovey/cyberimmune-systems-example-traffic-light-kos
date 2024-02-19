#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rtl/string.h>
#include <rtl/stdio.h>

#include "messages.h"

void send_diagnostic_message(TransportDescriptor *desc, u_int32_t in_code, char *in_message, char *entityName) {
    if (in_message == NULL) return;

    int logMessageLength = 0;
    char logMessage[traffic_light_IDiagMessage_Write_req_inMessage_message_size];
    rtl_memset(logMessage, 0, traffic_light_IDiagMessage_Write_req_inMessage_message_size);

    nk_arena_reset(desc->reqArena);

    nk_ptr_t *message = nk_arena_alloc(nk_ptr_t, desc->reqArena, &(desc->req->inMessage.message), 1);
    if (message == RTL_NULL) {
        fprintf(stderr, "[%-13s] ERR Can`t allocate memory in arena!\n", entityName);
        return;
    }
    logMessageLength = rtl_snprintf(logMessage, traffic_light_IDiagMessage_Write_req_inMessage_message_size, "%s", in_message);

    if (logMessageLength < 0) {
        fprintf(stderr, "[%-13s] ERR Length of message is negative number!\n", entityName);
        return;
    }

    nk_char_t *str = nk_arena_alloc(nk_char_t, desc->reqArena, &(message[0]), (nk_size_t) (logMessageLength + 1));
    if (str == RTL_NULL) {
        fprintf(stderr, "[%-13s] ERR Can`t allocate memory in arena!\n", entityName);
        return;
    }

    rtl_strncpy(str, logMessage, (rtl_size_t) (logMessageLength + 1));
    desc->req->inMessage.code = in_code;

    fprintf(stderr, "[%-13s] ==> HardwareDiagnostic [code: %08d, message: %s]\n", entityName, desc->req->inMessage.code, in_message);

    if (traffic_light_IDiagMessage_Write(&desc->proxy->base, desc->req, desc->reqArena, desc->res, desc->resArena) != NK_EOK) {
        fprintf(stderr, "[%-13s] ERR Can`t send message to HardwareDiagnostic entity!\n", entityName);
        return;
    } else {
        fprintf(stderr, "[%-13s] INF MESSAGE WROTE TO RECEIVER\n", entityName);
    }
}
