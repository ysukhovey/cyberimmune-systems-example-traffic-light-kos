
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <rtl/string.h>
#include <rtl/stdio.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* EDL description of the LightsGPIO entity. */
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

#include "messages.h"
#include "presentation.h"
#include <assert.h>

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
                           struct nk_arena *res_arena) {
    res->result = req->value;
    return NK_EOK;
}

/**
 * IMode object constructor.
 * step is the number by which the input value is increased.
 */
static struct traffic_light_IMode *CreateIModeImpl(rtl_uint32_t step) {
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

/* Lights GPIO entry point. */
int main(void) {
    NkKosTransport transport;
    ServiceId iid;

    /* Get lights gpio IPC handle of "lights_gpio_connection". */
    Handle handle = ServiceLocatorRegister("mc_gpio_connection", NULL, 0, &iid);
    assert(handle != INVALID_HANDLE);

    /* Initialize transport to control system. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
     * Prepare the structures of the request to the lights gpio entity: constant
     * part and arena. Because none of the methods of the lights gpio entity has
     * sequence type arguments, only constant parts of the
     * request and response are used. Arenas are effectively unused. However,
     * valid arenas of the request and response must be passed to
     * lights gpio transport methods (nk_transport_recv, nk_transport_reply) and
     * to the lights gpio method.
     */
    traffic_light_LightsGPIO_entity_req req;
    char req_buffer[traffic_light_LightsGPIO_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer, req_buffer + sizeof(req_buffer));

    /* Prepare response structures: constant part and arena. */
    traffic_light_LightsGPIO_entity_res res;
    char res_buffer[traffic_light_LightsGPIO_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer, res_buffer + sizeof(res_buffer));

    /**
     * Initialize mode component dispatcher. 3 is the value of the step,
     * which is the number by which the input value is increased.
     */
    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(0x1000000));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_LightsGPIO_entity entity;
    traffic_light_LightsGPIO_entity_init(&entity, &component);

    // HardwareDiagnostic connection init [START]
    NkKosTransport hd_transport;
    struct traffic_light_IDiagMessage_proxy hd_proxy;
    Handle hd_handle = ServiceLocatorConnect("gpio_diag_connection");
    if (hd_handle == INVALID_HANDLE) {
        fprintf(stderr, "[LightsGPIO   ] ERR Can`t establish static IPC connection!\n");
        return EXIT_FAILURE;
    }
    NkKosTransport_Init(&hd_transport, hd_handle, NK_NULL, 0);
    nk_iid_t hd_riid = ServiceLocatorGetRiid(hd_handle, "traffic_light.HardwareDiagnostic.write");
    if (hd_riid == INVALID_RIID) {
        fprintf(stderr, "[LightsGPIO   ] ERR Can`t get runtime implementation ID (RIID) of interface traffic_light.HardwareDiagnostic.write!\n");
        return EXIT_FAILURE;
    }
    traffic_light_IDiagMessage_proxy_init(&hd_proxy, &hd_transport.base, hd_riid);
    traffic_light_IDiagMessage_Write_req hd_req;
    traffic_light_IDiagMessage_Write_res hd_res;

    // HardwareDiagnostic connection init [END]

    fprintf(stderr, "[LightsGPIO   ] OK\n");

    char bs1[9], bs2[9], bs3[9], bs4[9];

    char ctl1[128], ctl2[128], ctl3[128], ctl4[128];
    rtl_memset(ctl1, 0, 128);
    rtl_memset(ctl2, 0, 128);
    rtl_memset(ctl3, 0, 128);
    rtl_memset(ctl4, 0, 128);


    /* Dispatch loop implementation. */
    do {
        /* Flush request/response buffers. */
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&transport.base, &req.base_, &req_arena) != NK_EOK) {
            fprintf(stderr, "[LightsGPIO   ] nk_transport_recv error\n");
        } else {
            /**
             * Handle received request by calling implementation Mode_impl
             * of the requested Mode interface method.
             */
            format_traffic_lights(((u_int8_t*)&req.lightsGpio_mode.FMode.value)[0], (char *)&bs1);
            format_traffic_lights(((u_int8_t*)&req.lightsGpio_mode.FMode.value)[1], (char *)&bs2);
            format_traffic_lights(((u_int8_t*)&req.lightsGpio_mode.FMode.value)[2], (char *)&bs3);
            format_traffic_lights(((u_int8_t*)&req.lightsGpio_mode.FMode.value)[3], (char *)&bs4);

            fprintf(stderr, "[LightsGPIO   ] GOT %08x |%s|%s|%s|%s|\n",
                    (rtl_uint32_t) req.lightsGpio_mode.FMode.value,
                    colorize_traffic_lights(bs1, ctl1),
                    colorize_traffic_lights(bs2, ctl2),
                    colorize_traffic_lights(bs3, ctl3),
                    colorize_traffic_lights(bs4, ctl4)
            );

            traffic_light_LightsGPIO_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
        }

        char reqBuffer[traffic_light_IDiagMessage_Write_req_arena_size];
        struct nk_arena hd_reqArena = NK_ARENA_INITIALIZER(reqBuffer, reqBuffer + sizeof(reqBuffer));
        TransportDescriptor desc = DESCR_INIT(&hd_proxy, &hd_req, &hd_res, &hd_reqArena, RTL_NULL);

        char buffer[traffic_light_IDiagMessage_Write_req_arena_size];
        rtl_memset(buffer, 0, traffic_light_IDiagMessage_Write_req_arena_size);
        rtl_snprintf(buffer, traffic_light_IDiagMessage_Write_req_arena_size - 1,
                     "{traffic_lights: ['%s', '%s', '%s', '%s'], mode: %08x}",
                     (char *)&bs1, (char *)&bs2, (char *)&bs3, (char *)&bs4, (rtl_uint32_t) req.lightsGpio_mode.FMode.value);
        send_diagnostic_message(&desc, (u_int32_t)rand(), buffer, "LightsGPIO");

        /* Send response. */
        uint32_t resReply = nk_transport_reply(&transport.base, &res.base_, &res_arena);
        if (resReply != NK_EOK) {
            fprintf(stderr, "[LightsGPIO   ] nk_transport_reply error [%d]\n", resReply);
        }
    } while (true);

    return EXIT_SUCCESS;
}
