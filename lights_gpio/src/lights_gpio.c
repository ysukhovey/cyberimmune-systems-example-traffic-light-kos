
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
#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

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
    Handle handle = ServiceLocatorRegister("mc_gpio_connection", NULL, 0, &iid);
    assert(handle != INVALID_HANDLE);
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);
    traffic_light_LightsGPIO_entity_req req;
    char req_buffer[traffic_light_LightsGPIO_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer, req_buffer + sizeof(req_buffer));
    traffic_light_LightsGPIO_entity_res res;
    char res_buffer[traffic_light_LightsGPIO_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer, res_buffer + sizeof(res_buffer));
    traffic_light_CMode_component component;
    traffic_light_CMode_component_init(&component, CreateIModeImpl(0x1000000));

    traffic_light_LightsGPIO_entity entity;
    traffic_light_LightsGPIO_entity_init(&entity, &component);
    fprintf(stderr, "[LightsGPIO   ] LightsGPIO service transport OK\n");

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
    fprintf(stderr, "[LightsGPIO   ] HardwareDiagnostic client transport OK\n");

    fprintf(stderr, "[LightsGPIO   ] OK\n");

    char bs1[9], bs2[9], bs3[9], bs4[9];

    char ctl1[128], ctl2[128], ctl3[128], ctl4[128];
    rtl_memset(ctl1, 0, 128);
    rtl_memset(ctl2, 0, 128);
    rtl_memset(ctl3, 0, 128);
    rtl_memset(ctl4, 0, 128);


    do {
        nk_req_reset(&req);
        nk_arena_reset(&req_arena);
        nk_arena_reset(&res_arena);

        if (nk_transport_recv(&transport.base, &req.base_, &req_arena) != NK_EOK) {
            fprintf(stderr, "[LightsGPIO   ] nk_transport_recv error\n");
        } else {
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
        }

        char reqBuffer[traffic_light_IDiagMessage_Write_req_arena_size];
        struct nk_arena hd_reqArena = NK_ARENA_INITIALIZER(reqBuffer, reqBuffer + sizeof(reqBuffer));

        char buffer[traffic_light_IDiagMessage_Write_req_arena_size];
        rtl_memset(buffer, 0, traffic_light_IDiagMessage_Write_req_arena_size);
        rtl_snprintf(buffer, traffic_light_IDiagMessage_Write_req_arena_size - 1,
                     "{traffic_lights: ['%s', '%s', '%s', '%s'], mode: %08x}",
                     (char *)&bs1, (char *)&bs2, (char *)&bs3, (char *)&bs4, (rtl_uint32_t) req.lightsGpio_mode.FMode.value);

        nk_arena_reset(&hd_reqArena);
        int logMessageLength = 0;
        char logMessage[traffic_light_IDiagMessage_Write_req_inMessage_message_size];
        rtl_memset(logMessage, 0, traffic_light_IDiagMessage_Write_req_inMessage_message_size);
        nk_ptr_t *message = nk_arena_alloc(nk_ptr_t, &hd_reqArena, &(hd_req.inMessage.message), 1);
        if (message == RTL_NULL) {
            fprintf(stderr, "[LightsGPIO   ] ERR Can`t allocate memory in the request arena!\n");
        } else {
            logMessageLength = rtl_snprintf(logMessage, traffic_light_IDiagMessage_Write_req_inMessage_message_size, "%s", buffer);

            if (logMessageLength < 0) {
                fprintf(stderr, "[LightsGPIO   ] ERR Length of message is a negative number!\n");
            } else {
                nk_char_t *str = nk_arena_alloc(nk_char_t, &hd_reqArena, &(message[0]), (nk_size_t)(logMessageLength + 1));
                if (str == RTL_NULL) {
                    fprintf(stderr, "[LightsGPIO   ] ERR Can`t allocate memory in the request arena!\n");
                } else {
                    rtl_strncpy(str, logMessage, (rtl_size_t)(logMessageLength + 1));
                    // todo REAL CODE!
                    hd_req.inMessage.code = (uint32_t) rand();

                    fprintf(stderr, "[LightsGPIO   ] ==> HardwareDiagnostic [code: %08d, message: %s]\n", hd_req.inMessage.code, buffer);

                    uint32_t send_result = traffic_light_IDiagMessage_Write(&hd_proxy.base, &hd_req, &hd_reqArena, &hd_res, NULL);
                    if (send_result != NK_EOK) {
                        fprintf(stderr, "[LightsGPIO   ] ERR Can`t send message to the HardwareDiagnostic entity!\n");
                    } else {
                        uint32_t resReply = nk_transport_reply(&transport.base, &res.base_, &res_arena);
                        if (resReply != NK_EOK) {
                            fprintf(stderr, "[LightsGPIO   ] nk_transport_reply error [%d]\n", resReply);
                        }
                    }
                }
            }
        }
        traffic_light_LightsGPIO_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
    } while (true);

    return EXIT_SUCCESS;
}
