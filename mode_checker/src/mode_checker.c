
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* EDL description of the LightsGPIO entity. */
#include <traffic_light/ModeChecker.edl.h>
/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/IMode.idl.h>

#include <assert.h>
#include "mode_list.h"

#define ANSI_COLOR_RED   "\x1b[91m"
#define ANSI_COLOR_GREEN "\x1b[92m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_BLINK_START "\x1b[5m"
#define ANSI_BLINK_RESET "\x1b[25m"

static rtl_uint32_t check_combination(rtl_uint32_t combination) {
    size_t length = sizeof(TRAFFIC_LIGHT_MODES) / sizeof(TRAFFIC_LIGHT_MODES[0]);;
    for (size_t i = 0; i < length; i++) {
        if (TRAFFIC_LIGHT_MODES[i] == combination) {
            return combination;
        }
    }
    return traffic_light_IMode_WRONGCOMBO;
}

typedef struct IModeImpl {
    struct traffic_light_IMode base;
    rtl_uint32_t step;
} IModeImpl;

static nk_err_t FMode_impl(struct traffic_light_IMode *self,
                           const struct traffic_light_IMode_FMode_req *req,
                           const struct nk_arena *req_arena,
                           traffic_light_IMode_FMode_res *res,
                           struct nk_arena *res_arena) {
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

int main(int argc, const char *argv[]) {
    NkKosTransport mc_transport;
    ServiceId mc_iid;
    Handle mc_handle = ServiceLocatorRegister("cs_mc_connection", NULL, 0, &mc_iid);
    assert(mc_handle != INVALID_HANDLE);
    NkKosTransport_Init(&mc_transport, mc_handle, NK_NULL, 0);
    traffic_light_ModeChecker_entity_req mc_req;
    char mc_req_buffer[traffic_light_ModeChecker_entity_req_arena_size];
    struct nk_arena mc_req_arena = NK_ARENA_INITIALIZER(mc_req_buffer, mc_req_buffer + sizeof(mc_req_buffer));
    traffic_light_ModeChecker_entity_res mc_res;
    char mc_res_buffer[traffic_light_ModeChecker_entity_res_arena_size];
    struct nk_arena mc_res_arena = NK_ARENA_INITIALIZER(mc_res_buffer, mc_res_buffer + sizeof(mc_res_buffer));
    traffic_light_CMode_component mc_component;
    traffic_light_CMode_component_init(&mc_component, CreateIModeImpl(traffic_light_IMode_WRONGCOMBO));
    traffic_light_ModeChecker_entity mc_entity;
    traffic_light_ModeChecker_entity_init(&mc_entity, &mc_component);
    fprintf(stderr, "[ModeChecker  ] ModeChecker IPC service transport (iid=%d) OK\n", mc_iid);

    NkKosTransport transport_lights_gpio;
    struct traffic_light_IMode_proxy proxy_lights_gpio;
    Handle handle_lights_gpio = ServiceLocatorConnect("mc_gpio_connection");
    assert(handle_lights_gpio != INVALID_HANDLE);
    NkKosTransport_Init(&transport_lights_gpio, handle_lights_gpio, NK_NULL, 0);
    nk_iid_t riid = ServiceLocatorGetRiid(handle_lights_gpio, "lightsGpio.mode");
    assert(riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&proxy_lights_gpio, &transport_lights_gpio.base, riid);
    traffic_light_IMode_FMode_req req_lights_gpio;
    traffic_light_IMode_FMode_res res_lights_gpio;
    fprintf(stderr, "[ModeChecker  ] LightsGPIO IPC client transport (riid=%d) OK\n", riid);

    fprintf(stderr, "[ModeChecker  ] OK\n");

    do {
        /* Flush request/response buffers. */
        nk_req_reset(&mc_req);
        nk_req_reset(&mc_res);
        nk_arena_reset(&mc_req_arena);
        nk_arena_reset(&mc_res_arena);

        // Dispatch requests
        uint32_t dispatch_result = traffic_light_ModeChecker_entity_dispatch(&mc_entity, &mc_req.base_, &mc_req_arena, &mc_res.base_, &mc_res_arena);
        if (dispatch_result != NK_EOK) {
            fprintf(stderr, "[ModeChecker  ] ERR dispatch() error (%d)\n", dispatch_result);
        }

        /* Wait for request to lights gpio entity. */
        uint32_t mc_rec_result = nk_transport_recv(&mc_transport.base, &mc_req.base_, &mc_req_arena);
        if (mc_rec_result == NK_EOK) {
            fprintf(stderr, "[ModeChecker  ] GOT %08x\n", (rtl_uint32_t) mc_req.modeChecker_mode.FMode.value);
            req_lights_gpio.value = check_combination(mc_req.modeChecker_mode.FMode.value);
            if (traffic_light_IMode_WRONGCOMBO == req_lights_gpio.value) {
                mc_res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
                fprintf(stderr, "[ModeChecker  ] CHK %sFAIL%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            }  else {
                fprintf(stderr, "[ModeChecker  ] CHK %sOK%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
                fprintf(stderr, "[ModeChecker  ] ==> LightsGPIO %08x\n", (rtl_uint32_t) req_lights_gpio.value);
                uint32_t tl_call_result = traffic_light_IMode_FMode(&proxy_lights_gpio.base, &req_lights_gpio, NULL, &res_lights_gpio, NULL);
                if (tl_call_result == rcOk) {
                    fprintf(stderr, "[ModeChecker  ] <== %08x\n", (rtl_uint32_t) res_lights_gpio.result);
                    mc_res.modeChecker_mode.FMode.result = res_lights_gpio.result;
                } else {
                    fprintf(stderr, "[ModeChecker  ] Failed to call traffic_light.Mode.Mode. (%d)\n", tl_call_result);
                    mc_res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
                }
            }
        }

        uint32_t reply_result = nk_transport_reply(&mc_transport.base, &mc_res.base_, &mc_res_arena);
        if (reply_result != NK_EOK) {
            fprintf(stderr, "[ModeChecker  ] ERR nk_transport_reply() error (%d)\n", reply_result);
        }

    } while (true);

    return EXIT_SUCCESS;
}