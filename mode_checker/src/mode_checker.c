
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

/* Type of interface implementing object. */
typedef struct IModeImpl {
    struct traffic_light_IMode base;     /* Base interface of object */
    rtl_uint32_t step;                   /* Extra parameters */
} IModeImpl;

/* Mode method implementation. */
static nk_err_t FMode_impl(struct traffic_light_IMode *self,
                           const struct traffic_light_IMode_FMode_req *req,
                           const struct nk_arena *req_arena,
                           traffic_light_IMode_FMode_res *res,
                           struct nk_arena *res_arena)
{
    res->result = req->value;
    return NK_EOK;
}

/**
 * IMode object constructor.
 * step is the number by which the input value is increased.
 */
static struct traffic_light_IMode *CreateIModeImpl(rtl_uint32_t step)
{
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_IMode_ops ops = {
            .FMode = FMode_impl
    };

    /* Interface implementing object. */
    static struct IModeImpl impl = {
            .base = {&ops}
    };

    impl.step = step;

    return &impl.base;
}

int main(int argc, const char *argv[]) {
    NkKosTransport mode_checker_transport;
    ServiceId iid;

    /* Get mode checker IPC handle of "mode_checker_connection". */
    Handle handle_mode_checker = ServiceLocatorRegister("mode_checker_connection", NULL, 0, &iid);
    assert(handle_mode_checker != INVALID_HANDLE);

    /* Initialize transport to control system. */
    NkKosTransport_Init(&mode_checker_transport, handle_mode_checker, NK_NULL, 0);

    /**
        * Prepare the structures of the request to the lights gpio entity: constant
        * part and arena. Because none of the methods of the lights gpio entity has
        * sequence type arguments, only constant parts of the
        * request and response are used. Arenas are effectively unused. However,
        * valid arenas of the request and response must be passed to
        * lights gpio transport methods (nk_transport_recv, nk_transport_reply) and
        * to the lights gpio method.
        */
    traffic_light_ModeChecker_entity_req req;
    char req_buffer[traffic_light_ModeChecker_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer,
                                                     req_buffer + sizeof(req_buffer));

    /* Prepare response structures: constant part and arena. */
    traffic_light_ModeChecker_entity_res res;
    char res_buffer[traffic_light_ModeChecker_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                                     res_buffer + sizeof(res_buffer));

    /**
     * Initialize mode component dispatcher. 3 is the value of the step,
     * which is the number by which the input value is increased.
     */
    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(traffic_light_IMode_WRONGCOMBO));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_ModeChecker_entity entity;
    traffic_light_ModeChecker_entity_init(&entity, &component);

    NkKosTransport transport_lights_gpio;
    struct traffic_light_IMode_proxy proxy_lights_gpio;

    Handle handle_lights_gpio = ServiceLocatorConnect("lights_gpio_connection");
    assert(handle_lights_gpio != INVALID_HANDLE);

    NkKosTransport_Init(&transport_lights_gpio, handle_lights_gpio, NK_NULL, 0);

    nk_iid_t riid = ServiceLocatorGetRiid(handle_lights_gpio, "lightsGpio.mode");
    assert(riid != INVALID_RIID);

    traffic_light_IMode_proxy_init(&proxy_lights_gpio, &transport_lights_gpio.base, riid);

    traffic_light_IMode_FMode_req req_lights_gpio;
    traffic_light_IMode_FMode_res res_lights_gpio;


    fprintf(stderr, "[ModeChecker  ] OK\n");

    /* Dispatch loop implementation. */
    do
    {
        /* Flush request/response buffers. */
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&mode_checker_transport.base, &req.base_, &req_arena) != NK_EOK) {
            fprintf(stderr, "[ModeChecker  ] nk_transport_recv error\n");
        } else {
            fprintf(stderr, "[ModeChecker  ] GOT %08x\n", (rtl_uint32_t) req.modeChecker_mode.FMode.value);
            req_lights_gpio.value = check_combination(req.modeChecker_mode.FMode.value);
            if (traffic_light_IMode_WRONGCOMBO == req_lights_gpio.value) {
                traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
                res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
                fprintf(stderr, "[ModeChecker  ] CHK %sFAIL%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
            }  else {
                fprintf(stderr, "[ModeChecker  ] CHK %sOK%s\n", ANSI_COLOR_GREEN, ANSI_COLOR_RESET);
                fprintf(stderr, "[ModeChecker  ] ==> LightsGPIO %08x\n", (rtl_uint32_t) req_lights_gpio.value);
                if (traffic_light_IMode_FMode(&proxy_lights_gpio.base, &req_lights_gpio, NULL, &res_lights_gpio,
                                              NULL) == rcOk) {
                    fprintf(stderr, "[ModeChecker  ] <== %08x\n", (rtl_uint32_t) res_lights_gpio.result);
                    req_lights_gpio.value = res_lights_gpio.result;
                    traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
                } else {
                    fprintf(stderr, "[ModeChecker  ] Failed to call traffic_light.Mode.Mode()\n");
                    traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
                    res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
                }
            }
        }

        /* Send response. */

        if (nk_transport_reply(&mode_checker_transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[ModeChecker  ] nk_transport_reply error\n");
        }

    }
    while (true);

    return EXIT_SUCCESS;
}