
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#include <traffic_light/HardwareDiagnostic.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

#include <assert.h>

typedef struct IDiagMessageImpl {
    struct traffic_light_IDiagMessage base;
} IDiagMessageImpl;

static nk_err_t FDiagMessage_impl(struct traffic_light_IDiagMessage *self,
                           const struct traffic_light_IDiagMessage_FDiagMessage_req *req,
                           const struct nk_arena *req_arena,
                           traffic_light_IDiagMessage_FDiagMessage_res *res,
                           struct nk_arena *res_arena)
{
    // todo Add proper implementation res->result = req->value;
    return NK_EOK;
}

static struct traffic_light_IDiagMessage *CreateIDiagMessageImpl(rtl_uint32_t _code, char *_message) {
    /* Table of implementations of IMode interface methods. */
    static const struct traffic_light_IDiagMessage_ops ops = {
            .FDiagMessage = FDiagMessage_impl
    };

    /* Interface implementing object. */
    static struct IDiagMessageImpl impl = {
            .base = {&ops}
    };

    // todo Add proper implementation impl.step = step;

    return &impl.base;
}

int main(int argc, const char *argv[]) {
    NkKosTransport hw_diag_transport;
    ServiceId iid;

    /* Get mode checker IPC handle of "mode_checker_connection". */
    Handle hw_diag_handle = ServiceLocatorRegister("gpio_diag_connection", NULL, 0, &iid);
    assert(hw_diag_handle != INVALID_HANDLE);

    /* Initialize transport to control system. */
    NkKosTransport_Init(&hw_diag_transport, hw_diag_handle, NK_NULL, 0);

    /**
        * Prepare the structures of the request to the lights gpio entity: constant
        * part and arena. Because none of the methods of the lights gpio entity has
        * sequence type arguments, only constant parts of the
        * request and response are used. Arenas are effectively unused. However,
        * valid arenas of the request and response must be passed to
        * lights gpio transport methods (nk_transport_recv, nk_transport_reply) and
        * to the lights gpio method.
        */
    traffic_light_HardwareDiagnostic_entity_req req;
    char req_buffer[traffic_light_HardwareDiagnostic_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer,
                                                     req_buffer + sizeof(req_buffer));

    /* Prepare response structures: constant part and arena. */
    traffic_light_HardwareDiagnostic_entity_res res;
    char res_buffer[traffic_light_HardwareDiagnostic_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                                     res_buffer + sizeof(res_buffer));

    /**
     * Initialize mode component dispatcher. 3 is the value of the step,
     * which is the number by which the input value is increased.
     */
    traffic_light_CDiagMessage_component component;
    traffic_light_CDiagMessage_component_init(&component, CreateIDiagMessageImpl(0, NULL));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_HardwareDiagnostic_entity entity;
    traffic_light_HardwareDiagnostic_entity_init(&entity, &component);

    NkKosTransport cs_transport;
    struct traffic_light_IDiagMessage_proxy cs_proxy;

    Handle cs_handle = ServiceLocatorConnect("diag_cs_connection");
    assert(cs_handle != INVALID_HANDLE);

    NkKosTransport_Init(&cs_transport, cs_handle, NK_NULL, 0);

    nk_iid_t riid = ServiceLocatorGetRiid(cs_handle, "hwDiag.message");
    assert(riid != INVALID_RIID);

    traffic_light_IDiagMessage_proxy_init(&cs_proxy, &cs_transport.base, riid);

    struct traffic_light_IDiagMessage_FDiagMessage_req cs_req;
    traffic_light_IDiagMessage_FDiagMessage_res cs_res;


    fprintf(stderr, "[HardwareDiag ] OK\n");

    /* Dispatch loop implementation. */
    do
    {
        /* Flush request/response buffers. */
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&hw_diag_transport.base,
                              &req.base_,
                              &req_arena) != NK_EOK) {
            fprintf(stderr, "[HardwareDiag ] nk_transport_recv error\n");
        } else {
            // todo implement call to the ControlSystem
            nk_char_t *message = RTL_NULL;
            nk_uint32_t message_len = 0;
            message = nk_arena_get(nk_char_t, &req_arena, &req.hwDiag_message.FDiagMessage.inbound.message, &message_len);

            fprintf(stderr, "[HardwareDiag ] GOT %08x : %s\n", req.hwDiag_message.FDiagMessage.inbound.code, message);
//            req_lights_gpio.value = check_combination(req.modeChecker_mode.FMode.value);
/*
            if (traffic_light_IMode_WRONGCOMBO == req_lights_gpio.value) {
                traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
                res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
                fprintf(stderr, "[ModeChecker  ] CHK FAIL\n");
            }  else {
                fprintf(stderr, "[ModeChecker  ] CHK OK\n");
                fprintf(stderr, "[ModeChecker  ] ==> %08x\n", (rtl_uint32_t) req_lights_gpio.value);
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
             */
        }

        /* Send response. */

        if (nk_transport_reply(&hw_diag_transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[HardwareDiag ] nk_transport_reply error\n");
        }

    }
    while (true);

    return EXIT_SUCCESS;
}