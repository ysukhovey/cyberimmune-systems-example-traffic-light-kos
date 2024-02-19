#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>
#include <assert.h>
#include "diag_msg.h"
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IMode.idl.h>
#include "../lights_gpio/src/messages.h"

// ControlSystem Transport Descriptor
struct traffic_light_IMode_proxy cs_proxy;
traffic_light_IMode_FMode_req cs_req;
traffic_light_IMode_FMode_res cs_res;

nk_err_t WriteImpl(__rtl_unused struct traffic_light_IDiagMessage          *self,
                         const traffic_light_IDiagMessage_Write_req        *req,
                         const struct nk_arena                             *reqArena,
                         __rtl_unused traffic_light_IDiagMessage_Write_res *res,
                         __rtl_unused struct nk_arena                      *resArena) {
    char *msg = WriteCommonImpl(self, req, reqArena, res, resArena, "HardwareDiag");
    if (msg != NULL) {
        cs_req.value = req->inMessage.code;
        fprintf(stderr,"[HardwareDiag ] ==> ControlSystem [code: %08d]\n", cs_req.value);
        uint32_t sendingResult = traffic_light_IMode_FMode(&cs_proxy.base, &cs_req, NULL, &cs_res, NULL);
        fprintf(stderr,"[HardwareDiag ] ==> ControlSystem Sent [code: %08d]\n", cs_req.value);
        if (sendingResult == rcOk) {
            //traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
        } else {
            fprintf(stderr,"[HardwareDiag ] ERR Failed to call ControlCenter.IMode.FMode(CODE)[%d]\n", sendingResult);
        }
        return NK_EOK;
    } else {
        return NK_EBADMSG;
    }
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
    char hwd_req_buffer[traffic_light_HardwareDiagnostic_entity_req_arena_size];
    struct nk_arena hwd_req_arena = NK_ARENA_INITIALIZER(hwd_req_buffer, hwd_req_buffer + sizeof(hwd_req_buffer));
    traffic_light_HardwareDiagnostic_entity_res hwd_res;
    traffic_light_HardwareDiagnostic_entity hwd_entity;
    traffic_light_HardwareDiagnostic_entity_init(&hwd_entity, CreateIDiagMessageImpl());

    // Transport infrastructure for ControlSystem connection
    NkKosTransport cs_transport;
    Handle cs_handle = ServiceLocatorConnect("diag_cs_connection");
    assert(cs_handle != INVALID_HANDLE);
    NkKosTransport_Init(&cs_transport, cs_handle, NK_NULL, 0);
    nk_iid_t cs_riid = ServiceLocatorGetRiid(cs_handle, "traffic_light.ControlSystem.code");
    assert(cs_riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&cs_proxy, &cs_transport.base, cs_riid);

    fprintf(stderr, "[HardwareDiag ] OK\n");

    do {
        nk_req_reset(&hwd_req);
        nk_arena_reset(&hwd_req_arena);

        if (nk_transport_recv(&hwd_transport.base, &hwd_req.base_, &hwd_req_arena) == NK_EOK) {
            traffic_light_HardwareDiagnostic_entity_dispatch(&hwd_entity, &hwd_req.base_, &hwd_req_arena, &hwd_res.base_, RTL_NULL);
        } else {
            //fprintf(stderr, "[HardwareDiag ] nk_transport_recv error\n");
        }

        if (nk_transport_reply(&hwd_transport.base, &hwd_res.base_, RTL_NULL) != NK_EOK) {
            //fprintf(stderr, "[HardwareDiag ]  nk_transport_reply error\n");
        }
    } while (true);

    return EXIT_SUCCESS;
}