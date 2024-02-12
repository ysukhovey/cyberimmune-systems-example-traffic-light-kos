
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#include <rtl/string.h>

#include <traffic_light/HardwareDiagnostic.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

#include <assert.h>

static nk_err_t WriteImpl(__rtl_unused struct traffic_light_IDiagMessage    *self,
                          const traffic_light_IDiagMessage_Write_req        *req,
                          const struct nk_arena                             *reqArena,
                          __rtl_unused traffic_light_IDiagMessage_Write_res *res,
                          __rtl_unused struct nk_arena                      *resArena) {

    nk_uint32_t msgCount = 0;

    nk_ptr_t *message = nk_arena_get(nk_ptr_t, reqArena, &(req->inMessage.message), &msgCount);
    if (message == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag1] Error: can`t get messages from arena!\n");
        return NK_EBADMSG;
    }

    nk_char_t *msg = RTL_NULL;
    nk_uint32_t msgLen = 0;
    msg = nk_arena_get(nk_char_t, reqArena, &message[0], &msgLen);
    if (msg == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag2] Error: can`t get message from arena!\n");
        return NK_EBADMSG;
    }

    fprintf(stderr, "[HardwareDiag3] GOT %08d: %s\n", req->inMessage.code, msg);

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
    NkKosTransport hwd_transport;
    ServiceId hwd_iid;

    Handle hwd_handle = ServiceLocatorRegister("gpio_diag_connection", NULL, 0, &hwd_iid);
    assert(hwd_handle != INVALID_HANDLE);

    NkKosTransport_Init(&hwd_transport, hwd_handle, NK_NULL, 0);

    traffic_light_HardwareDiagnostic_entity_req hwd_req;
    char hwd_req_buffer[traffic_light_HardwareDiagnostic_entity_req_arena_size];
    struct nk_arena hwd_req_arena = NK_ARENA_INITIALIZER(hwd_req_buffer, hwd_req_buffer + sizeof(hwd_req_buffer));

    traffic_light_HardwareDiagnostic_entity_res hwd_res;
    char hwd_res_buffer[traffic_light_HardwareDiagnostic_entity_res_arena_size];
    struct nk_arena hwd_res_arena = NK_ARENA_INITIALIZER(hwd_res_buffer, hwd_res_buffer + sizeof(hwd_res_buffer));

    traffic_light_HardwareDiagnostic_entity hwd_entity;
    traffic_light_HardwareDiagnostic_entity_init(&hwd_entity, CreateIDiagMessageImpl());

    fprintf(stderr, "[HardwareDiag ] OK\n");

    char decodedMessage[traffic_light_HardwareDiagnostic_component_req_arena_size];

    do {
        nk_req_reset(&hwd_req);
        nk_arena_reset(&hwd_req_arena);

        if (nk_transport_recv(&hwd_transport.base, &hwd_req.base_, &hwd_req_arena) == NK_EOK) {
            traffic_light_HardwareDiagnostic_entity_dispatch(&hwd_entity, &hwd_req.base_, &hwd_req_arena, &hwd_res.base_, RTL_NULL);
        } else {
            fprintf(stderr, "[HardwareDiag ] nk_transport_recv error\n");
        }

        if (nk_transport_reply(&hwd_transport.base, &hwd_res.base_, RTL_NULL) != NK_EOK) {
            fprintf(stderr, "[HardwareDiag ]  nk_transport_reply error\n");
        }
    } while (true);

    return EXIT_SUCCESS;
}