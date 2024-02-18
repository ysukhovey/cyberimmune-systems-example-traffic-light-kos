#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#include <traffic_light/Exchange.edl.h>
#include <traffic_light/IMode.idl.h>
#include <traffic_light/IDiagMessage.idl.h>

#include <assert.h>

void init_http_server_connection() {

}

void request_data_from_http_server() {

}

uint32_t send_message_to_control_system(traffic_light_IDiagMessage_DiagnosticMessage message) {
    sprintf(stderr, "[Exchange     ] code: %08x, message: %s\n", message.code, message.message);
    return 0;
}

static nk_err_t WriteImpl(__rtl_unused struct traffic_light_IDiagMessage    *self,
                          const traffic_light_IDiagMessage_Write_req        *req,
                          const struct nk_arena                             *reqArena,
                          __rtl_unused traffic_light_IDiagMessage_Write_res *res,
                          __rtl_unused struct nk_arena                      *resArena) {
    nk_uint32_t msgCount = 0;

    nk_ptr_t *message = nk_arena_get(nk_ptr_t, reqArena, &(req->inMessage.message), &msgCount);
    if (message == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag ] Error: can`t get messages from arena!\n");
        return NK_EBADMSG;
    }

    nk_char_t *msg = RTL_NULL;
    nk_uint32_t msgLen = 0;
    msg = nk_arena_get(nk_char_t, reqArena, &message[0], &msgLen);
    if (msg == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag ] Error: can`t get message from arena!\n");
        return NK_EBADMSG;
    }

    fprintf(stderr, "[HardwareDiag ] GOT %08d: %s\n", req->inMessage.code, msg);

    return NK_EOK;
}

static struct traffic_light_IDiagMessage *CreateIDiagMessageImpl(void) {
    static const struct traffic_light_IDiagMessage_ops Ops = {
            .Write = WriteImpl
    };
    static traffic_light_IDiagMessage obj = {
            .ops = &Ops
    };
    return &obj;
}

int main(int argc, const char *argv[]) {

    fprintf(stderr, "[Exchange     ] OK\n");

    for(;;) {
        request_data_from_http_server();

    /*    traffic_light_IDiagMessage_DiagnosticMessage message = {
                .code = 0x77777777,
                .message = NK_NULL
        };
        send_message_to_control_system(message);
*/
        // todo 1. request messages from the http-server
        // todo 1.1. send mode to the ControlSystem
        // todo 2. receive (if any) status message from the ControlSystem
    }

    return EXIT_SUCCESS;
}