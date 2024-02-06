
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the lights gpio interface used by the `ControlSystem` entity. */
#include <traffic_light/IMode.idl.h>

#include <assert.h>

/* Control system entity entry point. */
int main(int argc, const char *argv[])
{
    NkKosTransport transport;
    struct traffic_light_IMode_proxy proxy;

    static const nk_uint32_t tl_modes[] = {
            traffic_light_IMode_R1,
            traffic_light_IMode_R1 + traffic_light_IMode_AR1,
            traffic_light_IMode_R1 + traffic_light_IMode_Y1,
            traffic_light_IMode_R1 + traffic_light_IMode_Y1 + traffic_light_IMode_AR1,
            traffic_light_IMode_R1 + traffic_light_IMode_B1,
            traffic_light_IMode_G1,
            traffic_light_IMode_G1 + traffic_light_IMode_AL1 + traffic_light_IMode_AR1,
            traffic_light_IMode_G1 + traffic_light_IMode_AR1,
            traffic_light_IMode_B1,
            traffic_light_IMode_R1 + traffic_light_IMode_G2,
            traffic_light_IMode_Y1 + traffic_light_IMode_B1 + traffic_light_IMode_Y2 + traffic_light_IMode_B2 + traffic_light_IMode_Y3 + traffic_light_IMode_B3 + traffic_light_IMode_Y4 + traffic_light_IMode_B4

    };

    size_t modesNum = sizeof(tl_modes) / sizeof(tl_modes[0]);

    fprintf(stderr, "[ControlSystem] OK\n");

    /**
     * Get the LightsGPIO IPC handle of the connection named
     * "lights_gpio_connection".
     */
    Handle handle = ServiceLocatorConnect("mode_checker_connection");
    assert(handle != INVALID_HANDLE);

    /* Initialize IPC transport for interaction with the lights gpio entity. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
     * Get Runtime Interface ID (RIID) for interface traffic_light.Mode.mode.
     * Here mode is the name of the traffic_light.Mode component instance,
     * traffic_light.Mode.mode is the name of the Mode interface implementation.
     */
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "modeChecker.mode");
    assert(riid != INVALID_RIID);

    /**
     * Initialize proxy object by specifying transport (&transport)
     * and lights gpio interface identifier (riid). Each method of the
     * proxy object will be implemented by sending a request to the lights gpio.
     */
    traffic_light_IMode_proxy_init(&proxy, &transport.base, riid);

    /* Request and response structures */
    traffic_light_IMode_FMode_req req;
    traffic_light_IMode_FMode_res res;

    /* Test loop. */
    for (int i = 0; i < modesNum; i++) {
        req.value = tl_modes[i];
        fprintf(stderr, "[ControlSystem] Request %04d ------------------------------------ STARTED\n", i);
        fprintf(stderr, "[ControlSystem] ==> %08x\n", (rtl_uint32_t) req.value);
        /**
         * Call Mode interface method.
         * Lights GPIO will be sent a request for calling Mode interface method
         * mode_comp.mode_impl with the value argument. Calling thread is locked
         * until a response is received from the lights gpio.
         */
        if (traffic_light_IMode_FMode(&proxy.base, &req, NULL, &res, NULL) == rcOk) {
            /**
             * Print result value from response
             * (result is the output argument of the Mode method).
             */
            fprintf(stderr, "[ControlSystem] <== %08x\n", (rtl_uint32_t) res.result);
            /**
             * Include received result value into value argument
             * to resend to lights gpio in next iteration.
             */
            req.value = res.result;
        }
        else
            fprintf(stderr, "[ControlSystem] Failed to call traffic_light.Mode.Mode()\n");
        fprintf(stderr, "[ControlSystem] Request %04d ----------------------------------- FINISHED\n", i);
    }

    return EXIT_SUCCESS;
}
