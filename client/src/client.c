
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

/* Files required for transport initialization. */
#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

/* Description of the server interface used by the `client` entity. */
#include <echo/Ping.idl.h>

#include <assert.h>

#define EXAMPLE_VALUE_TO_SEND 777

/* Client entity entry point. */
int main(int argc, const char *argv[])
{
    NkKosTransport transport;
    struct echo_Ping_proxy proxy;
    int i;

    fprintf(stderr, "Hello I'm client\n");

    /**
     * Get the client IPC handle of the connection named
     * "server_connection".
     */
    Handle handle = ServiceLocatorConnect("server_connection");
    assert(handle != INVALID_HANDLE);

    /* Initialize IPC transport for interaction with the server entity. */
    NkKosTransport_Init(&transport, handle, NK_NULL, 0);

    /**
     * Get Runtime Interface ID (RIID) for interface echo.Ping.ping.
     * Here ping is the name of the echo.Ping component instance,
     * echo.Ping.ping is the name of the Ping interface implementation.
     */
    nk_iid_t riid = ServiceLocatorGetRiid(handle, "echo.Ping.ping");
    assert(riid != INVALID_RIID);

    /**
     * Initialize proxy object by specifying transport (&transport)
     * and server interface identifier (riid). Each method of the
     * proxy object will be implemented by sending a request to the server.
     */
    echo_Ping_proxy_init(&proxy, &transport.base, riid);

    /* Request and response structures */
    echo_Ping_Ping_req req;
    echo_Ping_Ping_res res;

    /* Test loop. */
    req.value = EXAMPLE_VALUE_TO_SEND;
    for (i = 0; i < 10; ++i)
    {
        /**
         * Call Ping interface method.
         * Server will be sent a request for calling Ping interface method
         * ping_comp.ping_impl with the value argument. Calling thread is locked
         * until a response is received from the server.
         */
        if (echo_Ping_Ping(&proxy.base, &req, NULL, &res, NULL) == rcOk)

        {
            /**
             * Print result value from response
             * (result is the output argument of the Ping method).
             */
            fprintf(stderr, "result = %d\n", (int) res.result);
            /**
             * Include received result value into value argument
             * to resend to server in next iteration.
             */
            req.value = res.result;

        }
        else
            fprintf(stderr, "Failed to call echo.Ping.Ping()\n");
    }

    return EXIT_SUCCESS;
}
