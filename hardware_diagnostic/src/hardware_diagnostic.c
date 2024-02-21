#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <rtl/string.h>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>
#include <assert.h>

#include <traffic_light/HardwareDiagnostic.edl.h>
#include <traffic_light/IDiagMessage.idl.h>
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/ICode.idl.h>

// ControlSystem Transport Descriptor
NkKosTransport cs_transport;
struct traffic_light_ICode_proxy cs_proxy;
Handle cs_handle;
nk_iid_t cs_riid;
traffic_light_ICode_FCode_req cs_req;
traffic_light_ICode_FCode_res cs_res;
char cs_req_buffer[traffic_light_ICode_FCode_req_arena_size];
struct nk_arena cs_req_arena = NK_ARENA_INITIALIZER(cs_req_buffer, cs_req_buffer + sizeof(cs_req_buffer));
char cs_res_buffer[traffic_light_ICode_FCode_req_arena_size];
struct nk_arena cs_res_arena = NK_ARENA_INITIALIZER(cs_res_buffer, cs_res_buffer + sizeof(cs_res_buffer));

nk_err_t WriteImpl(__rtl_unused struct traffic_light_IDiagMessage          *self,
                         const traffic_light_IDiagMessage_Write_req        *req,
                         const struct nk_arena                             *reqArena,
                         __rtl_unused traffic_light_IDiagMessage_Write_res *res,
                         __rtl_unused struct nk_arena                      *resArena) {
    nk_uint32_t msgCount = 0;

    nk_ptr_t *message = nk_arena_get(nk_ptr_t, reqArena, &(req->inMessage.message), &msgCount);
    if (message == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag ] ERR Can`t get messages from arena!\n");
        return NK_EBADMSG;
    }

    nk_char_t *msg = RTL_NULL;
    nk_uint32_t msgLen = 0;
    msg = nk_arena_get(nk_char_t, reqArena, &message[0], &msgLen);
    if (msg == RTL_NULL) {
        fprintf(stderr, "[HardwareDiag ] ERR Can`t get message from arena!\n");
        return NK_EBADMSG;
    } else {
        fprintf(stderr,"[HardwareDiag ] GOT [code: %08d, message: %s]\n", req->inMessage.code, msg);
    }

    cs_req.value = req->inMessage.code;
    fprintf(stderr,"[HardwareDiag ] ==> ControlSystem [code: %08d]\n", cs_req.value);
    uint32_t sendingResult = traffic_light_ICode_FCode(&cs_proxy.base, &cs_req, &cs_req_arena, &cs_res, &cs_res_arena);
    fprintf(stderr,"[HardwareDiag ] ==> ControlSystem Sent [code: %08d]\n", cs_req.value);
    if (sendingResult == rcOk) {
        //traffic_light_HardwareDiagnostic_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
    } else {
        fprintf(stderr,"[HardwareDiag ] ERR Failed to call ControlCenter.IMode.FMode(CODE)[%d]\n", sendingResult);
    }

    return NK_EOK;
}

struct traffic_light_IDiagMessage *CreateIDiagMessageImpl(void) {
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
    char hwd_req_buffer[traffic_light_IDiagMessage_arena_size];
    struct nk_arena hwd_req_arena = NK_ARENA_INITIALIZER(hwd_req_buffer, hwd_req_buffer + sizeof(hwd_req_buffer));
    char hwd_res_buffer[traffic_light_IDiagMessage_arena_size];
    struct nk_arena hwd_res_arena = NK_ARENA_INITIALIZER(hwd_res_buffer, hwd_res_buffer + sizeof(hwd_res_buffer));
    traffic_light_HardwareDiagnostic_entity_res hwd_res;
    traffic_light_HardwareDiagnostic_entity hwd_entity;
    traffic_light_HardwareDiagnostic_entity_init(&hwd_entity, CreateIDiagMessageImpl());
    fprintf(stderr, "[HardwareDiag ] HardwareDiagnostic service transport OK\n");

    // Transport infrastructure for ControlSystem connection
    cs_handle = ServiceLocatorConnect("diag_cs_connection");
    assert(cs_handle != INVALID_HANDLE);
    NkKosTransport_Init(&cs_transport, cs_handle, NK_NULL, 0);
    cs_riid = ServiceLocatorGetRiid(cs_handle, "traffic_light.ControlSystem.code");
    assert(cs_riid != INVALID_RIID);
    traffic_light_ICode_proxy_init(&cs_proxy, &cs_transport.base, cs_riid);

    fprintf(stderr, "[HardwareDiag ] ControlSystem client transport OK\n");

    fprintf(stderr, "[HardwareDiag ] OK\n");

    do {
        nk_req_reset(&hwd_req);
        nk_arena_reset(&hwd_req_arena);

        uint32_t hwdRecv = nk_transport_recv(&hwd_transport.base, &hwd_req.base_, &hwd_req_arena);
        if (hwdRecv == NK_EOK) {
            uint32_t hwdRepl = nk_transport_reply(&hwd_transport.base, &hwd_res.base_, &hwd_res_arena);
            if (hwdRepl != NK_EOK) {
                fprintf(stderr, "[HardwareDiag ] nk_transport_reply error [%d]\n", hwdRepl);
            }
        } else {
            fprintf(stderr, "[HardwareDiag ] nk_transport_recv error [%d]\n", hwdRecv);
        }

        traffic_light_HardwareDiagnostic_entity_dispatch(&hwd_entity, &hwd_req.base_, &hwd_req_arena, &hwd_res.base_, RTL_NULL);
    } while (true);

    return EXIT_SUCCESS;
}