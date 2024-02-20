#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/ICode.idl.h>
#include <traffic_light/IMode.idl.h>
#include <assert.h>
#include <traffic_light/ControlSystem.edl.h>
#include <traffic_light/ModeChecker.edl.h>
#include <traffic_light/LightsGPIO.edl.h>

typedef struct IModeImpl {
    struct traffic_light_IMode base;
    rtl_uint32_t step;
} IModeImpl;

static nk_err_t FMode_impl(struct traffic_light_IMode                   *self,
                           const struct traffic_light_IMode_FMode_req   *req,
                           const struct nk_arena                        *req_arena,
                           traffic_light_IMode_FMode_res                *res,
                           struct nk_arena                              *res_arena) {
    res->result = req->value;
    return NK_EOK;
}

static struct traffic_light_IMode *CreateIModeImpl(rtl_uint32_t step) {
    static const struct traffic_light_IMode_ops ops = {
            .FMode = FMode_impl
    };
    static struct IModeImpl impl = {
            .base = {&ops}
    };
    impl.step = step;
    return &impl.base;
}

typedef struct ICodeImpl {
    struct traffic_light_ICode base;
} ICodeImpl;

static nk_err_t FCode_impl(struct traffic_light_ICode                   *self,
                           const struct traffic_light_ICode_FCode_req   *req,
                           const struct nk_arena                        *req_arena,
                           traffic_light_ICode_FCode_res                *res,
                           struct nk_arena                              *res_arena) {
    res->result = req->value;
    return NK_EOK;
}

static struct traffic_light_ICode *CreateICodeImpl() {
    static const struct traffic_light_ICode_ops ops = {
            .FCode = FCode_impl
    };
    static struct ICodeImpl impl = {
            .base = {&ops}
    };
    return &impl.base;
}

/* Control system entity entry point. */
int main(int argc, const char *argv[])
{
    //--------------
    // Transport infrastructure for HardwareDiagnostic messages
    NkKosTransport hwd_transport;
    ServiceId hwd_iid;
    Handle hwd_handle = ServiceLocatorRegister("diag_cs_connection", NULL, 0, &hwd_iid);
    assert(hwd_handle != INVALID_HANDLE);
    NkKosTransport_Init(&hwd_transport, hwd_handle, NK_NULL, 0);
    traffic_light_ICode_FCode_req hwd_req;
    char hwd_req_buffer[traffic_light_ICode_FCode_req_arena_size];
    struct nk_arena hwd_req_arena = NK_ARENA_INITIALIZER(hwd_req_buffer, hwd_req_buffer + sizeof(hwd_req_buffer));
    traffic_light_ICode_FCode_res hwd_res;
    char hwd_res_buffer[traffic_light_ICode_FCode_res_arena_size];
    struct nk_arena hwd_res_arena = NK_ARENA_INITIALIZER(hwd_res_buffer, hwd_res_buffer + sizeof(hwd_res_buffer));
    fprintf(stderr, "[ControlSystem] HardwareDiagnostic transport OK\n");

    // Transport infrastructure for Exchange messages
    NkKosTransport ex_transport;
    ServiceId ex_iid;
    Handle ex_handle = ServiceLocatorRegister("exchange_cs_connection", NULL, 0, &ex_iid);
    assert(ex_handle != INVALID_HANDLE);
    NkKosTransport_Init(&ex_transport, ex_handle, NK_NULL, 0);
    traffic_light_IMode_FMode_req ex_req;
    char ex_req_buffer[traffic_light_IMode_FMode_req_arena_size];
    struct nk_arena ex_req_arena = NK_ARENA_INITIALIZER(ex_req_buffer, ex_req_buffer + sizeof(ex_req_buffer));
    traffic_light_IMode_FMode_res ex_res;
    char ex_res_buffer[traffic_light_IMode_FMode_res_arena_size];
    struct nk_arena ex_res_arena = NK_ARENA_INITIALIZER(ex_res_buffer, ex_res_buffer + sizeof(ex_res_buffer));

    fprintf(stderr, "[ControlSystem] Exchange transport OK\n");

    //---------------
    NkKosTransport transport;
    struct traffic_light_IMode_proxy proxy;
    Handle handle = ServiceLocatorConnect("mode_checker_connection");
    assert(handle != INVALID_HANDLE);
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "modeChecker.mode");
    assert(riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&proxy, &transport.base, riid);
    traffic_light_IMode_FMode_req req;
    traffic_light_IMode_FMode_res res;
    char req_buffer[traffic_light_IMode_FMode_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer, req_buffer + sizeof(req_buffer));
    char res_buffer[traffic_light_IMode_FMode_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer, res_buffer + sizeof(res_buffer));
    fprintf(stderr, "[ControlSystem] ModeChecker transport OK\n");

    traffic_light_ControlSystem_entity cs_entity;
    traffic_light_ControlSystem_entity_init(&cs_entity, CreateICodeImpl(), CreateIModeImpl(0));

    fprintf(stderr, "[ControlSystem] OK\n");

    for(;;) {
        nk_req_reset(&ex_req);
        nk_arena_reset(&ex_req_arena);

        nk_req_reset(&hwd_req);
        nk_arena_reset(&hwd_req_arena);

        // Wait for request from Exchange
        if (nk_transport_recv(&ex_transport.base, &ex_req.base_, &ex_req_arena) == NK_EOK) {
            req.value = ex_req.value;
            fprintf(stderr, "[ControlSystem] ==> ModeChecker %08x\n", (rtl_uint32_t) req.value);
            traffic_light_IMode_FMode(&proxy.base, &req, NULL, &res, NULL) == rcOk;
            uint32_t ex_reply_result = nk_transport_reply(&ex_transport.base, &ex_res.base_, &ex_res_arena);
            if (ex_reply_result != NK_EOK) {
                fprintf(stderr, "[ControlSystem] Exchange nk_transport_reply error (%d)\n", ex_reply_result);
            }
        }

        // Wait for request from HardwareDiagnostic
        if (nk_transport_recv(&hwd_transport.base, &hwd_req.base_, &hwd_req_arena) == NK_EOK) {
            // todo Send data to the Exchange
            //traffic_light_HardwareDiagnostic_entity_dispatch(&cs_entity, &hwd_req.base_, &hwd_req_arena, &hwd_res.base_, RTL_NULL);
            //req.value = hwd_req.value;
            fprintf(stderr, "[ControlSystem] GOT Code from HardwareDiagnostic %08x\n", (rtl_uint32_t) hwd_req.value);
            //traffic_light_IMode_FMode(&proxy.base, &req, NULL, &res, NULL) == rcOk;
            uint32_t hwd_reply_result = nk_transport_reply(&hwd_transport.base, &hwd_res.base_, &hwd_res_arena);
            if (hwd_reply_result != NK_EOK) {
                fprintf(stderr, "[ControlSystem] HardwareDiagnostic nk_transport_reply error (%d)\n", hwd_reply_result);
            }
        }

        traffic_light_ModeChecker_entity_dispatch(&cs_entity, &req.base_, &req_arena, &res.base_, &res_arena);
    }

    return EXIT_SUCCESS;
}
