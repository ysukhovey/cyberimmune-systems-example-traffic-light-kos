
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

#include <assert.h>

#define ANSI_COLOR_BLACK    "\x1b[30m"
#define ANSI_COLOR_CYAN     "\x1b[36m"
#define ANSI_COLOR_RED      "\x1b[91m"
#define ANSI_COLOR_YELLOW   "\x1b[93m"
#define ANSI_COLOR_GREEN    "\x1b[92m"
#define ANSI_COLOR_RESET    "\x1b[0m"
#define ANSI_BLINK_ON       "\x1b[5m"
#define ANSI_BLINK_OFF      "\x1b[25m"

// --------------------------------------
typedef struct {
    struct traffic_light_IDiagMessage_proxy *proxy;
    traffic_light_IDiagMessage_Write_req *req;
    traffic_light_IDiagMessage_Write_res *res;
    struct nk_arena *reqArena;
    struct nk_arena *resArena;
} TransportDescriptor;

#define DESCR_INIT(proxyIn, reqIn, resIn, reqArenaIn, resArenaIn) \
        {                                           \
            .proxy = proxyIn,                       \
            .req = reqIn,                           \
            .res = resIn,                           \
            .reqArena = reqArenaIn,                 \
            .resArena = resArenaIn,                 \
        }

static void send_diagnostic_message(TransportDescriptor *desc, u_int32_t in_code, char *in_message) {
    if (in_message == NULL) return;

    int logMessageLength = 0;
    char logMessage[traffic_light_IDiagMessage_Write_req_inMessage_message_size];
    rtl_memset(logMessage, 0, traffic_light_IDiagMessage_Write_req_inMessage_message_size);

    nk_arena_reset(desc->reqArena);

    nk_ptr_t *message = nk_arena_alloc(nk_ptr_t, desc->reqArena, &(desc->req->inMessage.message), 1);
    if (message == RTL_NULL) {
        fprintf(stderr, "[LightsGPIO   ] %sError: can`t allocate memory in arena!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }

    logMessageLength = rtl_snprintf(logMessage, traffic_light_IDiagMessage_Write_req_inMessage_message_size, in_message);
    if (logMessageLength < 0) {
        fprintf(stderr, "[LightsGPIO   ] %sError: length of message is negative number!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }

    nk_char_t *str = nk_arena_alloc(nk_char_t, desc->reqArena, &(message[0]), (nk_size_t) (logMessageLength + 1));
    if (str == RTL_NULL) {
        fprintf(stderr, "[LightsGPIO   ] %sError: can`t allocate memory in arena!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }

    rtl_strncpy(str, logMessage, (rtl_size_t) (logMessageLength + 1));
    desc->req->inMessage.code = in_code;

    fprintf(stderr, "[LightsGPIO   ] ==> %08d: %s\n", desc->req->inMessage.code, in_message);

    if (traffic_light_IDiagMessage_Write(&desc->proxy->base, desc->req, desc->reqArena, desc->res, desc->resArena) != NK_EOK) {
        fprintf(stderr, "[LightsGPIO   ] %sError: can`t send message to HardwareDiagnostic entity!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return;
    }
}
// --------------------------------------


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


static char *blinkOn(char a, char b) {
    return ((b != '.') && (a != '.'))?ANSI_BLINK_ON:"";
}

static char *blinkOff(char a, char b) {
    return ((b != '.') && (a != '.'))?ANSI_BLINK_OFF:"";
}

// todo implement
static char* colorize_traffic_lights(char *binstr, char *colorized) {
    if (colorized == NULL) return "--------";

    sprintf(colorized, "%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s%s%c%s%s%s..%s%s%c%s",
            blinkOn(binstr[0], binstr[7]),
            (binstr[0] != '.')?ANSI_COLOR_RED:"",
            (binstr[0] != '.')?'R':'.',
            (binstr[0] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[0], binstr[7]),

            blinkOn(binstr[1], binstr[7]),
            (binstr[1] != '.')?ANSI_COLOR_YELLOW:"",
            (binstr[1] != '.')?'Y':'.',
            (binstr[1] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[1], binstr[7]),

            blinkOn(binstr[2], binstr[7]),
            (binstr[2] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[2] != '.')?'G':'.',
            (binstr[2] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[2], binstr[7]),

            blinkOn(binstr[3], binstr[7]),
            (binstr[3] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[3] != '.')?'<':'.',
            (binstr[3] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[3], binstr[7]),

            blinkOn(binstr[4], binstr[7]),
            (binstr[4] != '.')?ANSI_COLOR_GREEN:"",
            (binstr[4] != '.')?'>':'.',
            (binstr[4] != '.')?ANSI_COLOR_RESET:"",
            blinkOff(binstr[4], binstr[7]),

            blinkOn(binstr[5], binstr[7]),
            blinkOff(binstr[5], binstr[7]),

            (binstr[7] != '.')?ANSI_COLOR_CYAN:"",
            (binstr[7] != '.')?'B':'.',
            (binstr[7] != '.')?ANSI_COLOR_RESET:""

            );
    return colorized;
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

    // HardwareDiagnostic connection init [START]
    NkKosTransport hd_transport;
    struct traffic_light_IDiagMessage_proxy hd_proxy;
    Handle hd_handle = ServiceLocatorConnect("gpio_diag_connection");
    if (handle == INVALID_HANDLE) {
        fprintf(stderr, "[LightsGPIO   ] %sError: can`t establish static IPC connection!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
        return EXIT_FAILURE;
    }
    NkKosTransport_Init(&hd_transport, hd_handle, NK_NULL, 0);
    nk_iid_t hd_riid = ServiceLocatorGetRiid(hd_handle, "traffic_light.HardwareDiagnostic.write");
    if (hd_riid == INVALID_RIID) {
        fprintf(stderr, "[LightsGPIO   ] %sError: can`t get runtime implementation ID (RIID) of "
                        "interface traffic_light.HardwareDiagnostic.write!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
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

            fprintf(stderr, "[LightsGPIO   ] GOT %08x |%s|%s|%s|%s|\n",
                    (rtl_uint32_t) req.lightsGpio_mode.FMode.value,
                    colorize_traffic_lights(bs1, ctl1),
                    colorize_traffic_lights(bs2, ctl2),
                    colorize_traffic_lights(bs3, ctl3),
                    colorize_traffic_lights(bs4, ctl4)
            );

            traffic_light_LightsGPIO_entity_dispatch(&entity, &req.base_, &req_arena,
                                                     &res.base_, &res_arena);
        }

        // todo Add message sending to the Hardware Diagnostic

        char reqBuffer[traffic_light_IDiagMessage_Write_req_arena_size];
        struct nk_arena hd_reqArena = NK_ARENA_INITIALIZER(reqBuffer, reqBuffer + sizeof(reqBuffer));
        TransportDescriptor desc = DESCR_INIT(&hd_proxy, &hd_req, &hd_res, &hd_reqArena, RTL_NULL);

        char buffer[traffic_light_IDiagMessage_Write_req_arena_size];
        rtl_memset(buffer, 0, traffic_light_IDiagMessage_Write_req_arena_size);
        rtl_snprintf(buffer, traffic_light_IDiagMessage_Write_req_arena_size - 1,
                     "{traffic_lights: ['%s', '%s', '%s', '%s'], mode: %08x}",
                     (char *)&bs1, (char *)&bs2, (char *)&bs3, (char *)&bs4, (rtl_uint32_t) req.lightsGpio_mode.FMode.value);
        send_diagnostic_message(&desc, (u_int32_t)rand(), buffer);

        // todo END send_error_message call

        /* Send response. */
        if (nk_transport_reply(&transport.base,
                               &res.base_,
                               &res_arena) != NK_EOK) {
            fprintf(stderr, "[LightsGPIO   ] nk_transport_reply error\n");
        }
    } while (true);

    return EXIT_SUCCESS;
}
