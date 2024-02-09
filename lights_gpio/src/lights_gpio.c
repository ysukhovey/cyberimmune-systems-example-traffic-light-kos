
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* EDL description of the LightsGPIO entity. */
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

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



static void send_error_message(nk_uint32_t code, char *message) {
    NkKosTransport transport;
    struct traffic_light_IDiagMessage_proxy proxy;
    Handle handle = ServiceLocatorConnect("gpio_diag_connection");
    assert(handle != INVALID_HANDLE);
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "hwDiag.message");
    assert(riid != INVALID_RIID);
    traffic_light_IDiagMessage_proxy_init(&proxy, &transport.base, riid);

    traffic_light_IDiagMessage_FDiagMessage_req req;
    traffic_light_IDiagMessage_FDiagMessage_res res;

    char *staticMsg = "Message to demonstrate error description sending";

    const size_t bufSize = 128;

    char reqBuffer[bufSize];

    struct nk_arena reqArena = NK_ARENA_INITIALIZER(reqBuffer, reqBuffer + sizeof(reqBuffer));
    //todo add folding here
    nk_ptr_t *msg = nk_arena_alloc(nk_char_t,
                                   &reqArena,
                                   staticMsg,
                                   1);

    if (msg == RTL_NULL) {
        fprintf(stderr, "[LightsGPIO   ] Arena nk_ptr_t allocation failed\n");
        return;
    }



    req.inbound.code = 8;
    //req.inbound.message =

    if (traffic_light_IDiagMessage_FDiagMessage(&proxy.base, &req, NULL, &res, NULL) == rcOk) {
        fprintf(stderr, "[LightsGPIO   ] Error message sent to HardwareDiagnostic\n");
    }
    else
        fprintf(stderr, "[LIghtsGPIO   ] Failed to call traffic_light.Mode.Mode()\n");
}

/*
    Presentation functions
 */
static void format_traffic_lights(u_int8_t n, char *binstr) {
    memset(binstr, 0, 9);
    for (int i = 7; i >= 0; i--) {
        int k = n >> i;
        if (k & 1)
            binstr[7 - i] = '1';
        else
            binstr[7 - i] = '0';
    }

    binstr[0] = (binstr[0] == '1' ? 'R' : '.');
    binstr[1] = (binstr[1] == '1' ? 'Y' : '.');
    binstr[2] = (binstr[2] == '1' ? 'G' : '.');
    binstr[3] = (binstr[3] == '1' ? '<' : '.');
    binstr[4] = (binstr[4] == '1' ? '>' : '.');
    binstr[5] = (binstr[5] == '1' ? '?' : '.');
    binstr[6] = (binstr[6] == '1' ? '?' : '.');
    binstr[7] = (binstr[7] == '1' ? 'B' : '.');

}

/* Lights GPIO entry point. */
int main(void) {
    NkKosTransport transport;
    ServiceId iid;

    /* Get lights gpio IPC handle of "lights_gpio_connection". */
    Handle handle = ServiceLocatorRegister("lights_gpio_connection", NULL, 0, &iid);
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
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer,
                                                     req_buffer + sizeof(req_buffer));

    /* Prepare response structures: constant part and arena. */
    traffic_light_LightsGPIO_entity_res res;
    char res_buffer[traffic_light_LightsGPIO_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer,
                                                     res_buffer + sizeof(res_buffer));

    /**
     * Initialize mode component dispatcher. 3 is the value of the step,
     * which is the number by which the input value is increased.
     */
    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(0x1000000));

    /* Initialize lights gpio entity dispatcher. */
    traffic_light_LightsGPIO_entity entity;
    traffic_light_LightsGPIO_entity_init(&entity, &component);

    fprintf(stderr, "[LightsGPIO   ] OK\n");

    char bs1[9], bs2[9], bs3[9], bs4[9];

    /* Dispatch loop implementation. */
    do {
        /* Flush request/response buffers. */
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        /* Wait for request to lights gpio entity. */
        if (nk_transport_recv(&transport.base,
                              &req.base_,
                              &req_arena) != NK_EOK) {
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

            fprintf(stderr, "[LightsGPIO   ] GOT %08x |%s|%s|%s|%s|\n", (rtl_uint32_t) req.lightsGpio_mode.FMode.value,
                    bs1, bs2, bs3, bs4
            );

            traffic_light_LightsGPIO_entity_dispatch(&entity, &req.base_, &req_arena,
                                                     &res.base_, &res_arena);
        }

        /* Send response. */
        if (nk_transport_reply(&transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[LightsGPIO   ] nk_transport_reply error\n");
        }
    } while (true);

    return EXIT_SUCCESS;
}
