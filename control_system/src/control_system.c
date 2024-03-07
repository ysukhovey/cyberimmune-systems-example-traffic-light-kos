#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/ICode.idl.h>
#include <traffic_light/CCode.cdl.h>
#include <traffic_light/IMode.idl.h>
#include <traffic_light/CMode.cdl.h>
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

// Transport infrastructure for HardwareDiagnostic messages
NkKosTransport hwd_transport;
ServiceId hwd_iid;
Handle hwd_handle;
traffic_light_ICode_FCode_req hwd_req;
char hwd_req_buffer[traffic_light_ICode_FCode_req_arena_size];
struct nk_arena hwd_req_arena = NK_ARENA_INITIALIZER(hwd_req_buffer, hwd_req_buffer + sizeof(hwd_req_buffer));
traffic_light_ICode_FCode_res hwd_res;
char hwd_res_buffer[traffic_light_ICode_FCode_res_arena_size];
struct nk_arena hwd_res_arena = NK_ARENA_INITIALIZER(hwd_res_buffer, hwd_res_buffer + sizeof(hwd_res_buffer));
traffic_light_CCode_component hwd_component;

// Transport infrastructure for Exchange messages
NkKosTransport ex_transport;
ServiceId ex_iid;
Handle ex_handle;
traffic_light_IMode_FMode_req ex_req;
char ex_req_buffer[traffic_light_IMode_FMode_req_arena_size];
struct nk_arena ex_req_arena = NK_ARENA_INITIALIZER(ex_req_buffer, ex_req_buffer + sizeof(ex_req_buffer));
traffic_light_IMode_FMode_res ex_res;
char ex_res_buffer[traffic_light_IMode_FMode_res_arena_size];
struct nk_arena ex_res_arena = NK_ARENA_INITIALIZER(ex_res_buffer, ex_res_buffer + sizeof(ex_res_buffer));
traffic_light_CMode_component ex_component;

//
NkKosTransport ex_cl_transport;
struct traffic_light_IMode_proxy ex_cl_proxy;
Handle ex_cl_handle;
nk_iid_t ex_cl_riid;
traffic_light_IMode_FMode_req ex_cl_req;
traffic_light_IMode_FMode_res ex_cl_res;
char ex_cl_req_buffer[traffic_light_IMode_FMode_req_arena_size];
struct nk_arena ex_cl_req_arena = NK_ARENA_INITIALIZER(ex_cl_req_buffer, ex_cl_req_buffer + sizeof(ex_cl_req_buffer));
char ex_cl_res_buffer[traffic_light_IMode_FMode_res_arena_size];
struct nk_arena ex_cl_res_arena = NK_ARENA_INITIALIZER(ex_cl_res_buffer, ex_cl_res_buffer + sizeof(ex_cl_res_buffer));

// CS Entity
traffic_light_ControlSystem_entity cs_entity;

// Hardware Diagnostic listener thread ID
pthread_t hw_listener_id;

void *hardware_diagnostic_listener(void *vargp) {
    nk_req_reset(&hwd_req);
    nk_req_reset(&hwd_res);
    nk_arena_reset(&hwd_req_arena);
    nk_arena_reset(&hwd_res_arena);

    if (nk_transport_recv(&hwd_transport.base, &hwd_req.base_, &hwd_req_arena) == NK_EOK) {
        traffic_light_ControlSystem_entity_dispatch(&cs_entity, &hwd_req.base_, &hwd_req_arena, &hwd_res.base_, &hwd_res_arena);
        fprintf(stderr, "[ControlSystem] GOT Code from HardwareDiagnostic %08x\n", (rtl_uint32_t) hwd_req.value);
        fprintf(stderr, "[ControlSystem] ==> Exchange %08x\n", (rtl_uint32_t) hwd_req.value);
        ex_cl_req.value = hwd_req.value;
        uint32_t  ex_cl_call_result = traffic_light_ICode_FCode(&ex_cl_proxy.base, &ex_cl_req, NULL, &ex_cl_res, NULL);
        if (ex_cl_call_result == NK_EOK) {
            fprintf(stderr, "[ControlSystem] Sent %08x to Exchange\n", (rtl_uint32_t) hwd_req.value);
        } else {
            fprintf(stderr, "[ControlSystem] Exchange service client nk_transport_reply error (%d)\n", ex_cl_call_result);
        }
        uint32_t hwd_reply_result = nk_transport_reply(&hwd_transport.base, &hwd_res.base_, &hwd_res_arena);
        if (hwd_reply_result != NK_EOK) {
            fprintf(stderr, "[ControlSystem] HardwareDiagnostic service nk_transport_reply error (%d)\n", hwd_reply_result);
        }
    }
    return NULL;
}

/* Control system entity entry point. */
int main(int argc, const char *argv[]) {
    //--------------
    // Init transport infrastructure for HardwareDiagnostic messages
    hwd_handle = ServiceLocatorRegister("diag_cs_connection", NULL, 0, &hwd_iid);
    assert(hwd_handle != INVALID_HANDLE);
    NkKosTransport_Init(&hwd_transport, hwd_handle, NK_NULL, 0);
    traffic_light_CCode_component_init(&hwd_component, CreateICodeImpl());

    fprintf(stderr, "[ControlSystem] HardwareDiagnostic IPC service transport (iid=%d) OK\n", hwd_iid);

    // Init transport infrastructure for Exchange messages
    ex_handle = ServiceLocatorRegister("exchange_cs_connection", NULL, 0, &ex_iid);
    assert(ex_handle != INVALID_HANDLE);
    NkKosTransport_Init(&ex_transport, ex_handle, NK_NULL, 0);
    traffic_light_CMode_component_init(&ex_component, CreateIModeImpl(0));

    fprintf(stderr, "[ControlSystem] Exchange IPC service transport (iid=%d) OK\n", ex_iid);

    //---------------
    ex_cl_handle = ServiceLocatorConnect("cs_exchange_connection");
    assert(ex_cl_handle != INVALID_HANDLE);
    NkKosTransport_Init(&ex_cl_transport, ex_cl_handle, NK_NULL, 0);
    ex_cl_riid = ServiceLocatorGetRiid(ex_cl_handle, "traffic_light.Exchange.code");
    assert(ex_cl_riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&ex_cl_proxy, &ex_cl_transport.base, ex_cl_riid);
    fprintf(stderr, "[ControlSystem] Exception IPC client transport (riid=%d) OK\n", ex_cl_riid);

    //---------------
    NkKosTransport mc_transport;
    struct traffic_light_IMode_proxy mc_proxy;
    Handle mc_handle = ServiceLocatorConnect("cs_mc_connection");
    assert(mc_handle != INVALID_HANDLE);
    NkKosTransport_Init(&mc_transport, mc_handle, NK_NULL, 0);
    nk_iid_t mc_riid = ServiceLocatorGetRiid(mc_handle, "modeChecker.mode");
    assert(mc_riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&mc_proxy, &mc_transport.base, mc_riid);
    traffic_light_IMode_FMode_req mc_req;
    traffic_light_IMode_FMode_res mc_res;
    char mc_req_buffer[traffic_light_ModeChecker_entity_res_arena_size];
    struct nk_arena mc_req_arena = NK_ARENA_INITIALIZER(mc_req_buffer, mc_req_buffer + sizeof(mc_req_buffer));
    char mc_res_buffer[traffic_light_ModeChecker_entity_res_arena_size];
    struct nk_arena mc_res_arena = NK_ARENA_INITIALIZER(mc_res_buffer, mc_res_buffer + sizeof(mc_res_buffer));
    fprintf(stderr, "[ControlSystem] ModeChecker IPC client transport (riid=%d) OK\n", mc_riid);

    traffic_light_ControlSystem_entity_init(&cs_entity, &hwd_component, &ex_component);

    fprintf(stderr, "[ControlSystem] OK\n");

    pthread_create(&hw_listener_id, NULL, hardware_diagnostic_listener, NULL);

    for(;;) {
        // Clean requests/responses along with their arenas
        nk_req_reset(&ex_req);
        nk_req_reset(&ex_res);
        nk_arena_reset(&ex_req_arena);
        nk_arena_reset(&ex_res_arena);
        nk_req_reset(&mc_req);
        nk_req_reset(&mc_res);
        nk_arena_reset(&mc_req_arena);
        nk_arena_reset(&mc_res_arena);

        // Wait for request from Exchange
        if (nk_transport_recv(&ex_transport.base, &ex_req.base_, &ex_req_arena) == NK_EOK) {
            mc_req.value = ex_req.value;
            fprintf(stderr, "[ControlSystem] ==> ModeChecker %08x\n", (rtl_uint32_t) mc_req.value);
            uint32_t mc_call_result = traffic_light_IMode_FMode(&mc_proxy.base, &mc_req, &mc_req_arena, &mc_res, &mc_res_arena);
            if (mc_call_result  == rcOk) {
                traffic_light_ControlSystem_entity_dispatch(&cs_entity, &ex_req.base_, &ex_req_arena, &ex_res.base_, &ex_res_arena);
            }
        }
        uint32_t ex_reply_result = nk_transport_reply(&ex_transport.base, &ex_res.base_, &ex_res_arena);
        if (ex_reply_result != NK_EOK) {
            fprintf(stderr, "[ControlSystem] Exchange nk_transport_reply error (%d)\n", ex_reply_result);
        }
    }

    return EXIT_SUCCESS;
}
