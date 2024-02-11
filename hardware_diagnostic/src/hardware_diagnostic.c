
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

int main(int argc, const char *argv[]) {
    NkKosTransport hw_diag_transport;
    ServiceId iid;

    Handle hw_diag_handle = ServiceLocatorRegister("gpio_diag_connection", NULL, 0, &iid);
    assert(hw_diag_handle != INVALID_HANDLE);

    NkKosTransport_Init(&hw_diag_transport, hw_diag_handle, NK_NULL, 0);

    traffic_light_HardwareDiagnostic_entity_req req;
    char req_buffer[traffic_light_HardwareDiagnostic_entity_req_arena_size];
    struct nk_arena req_arena = NK_ARENA_INITIALIZER(req_buffer, req_buffer + sizeof(req_buffer));

    traffic_light_HardwareDiagnostic_entity_res res;
    char res_buffer[traffic_light_HardwareDiagnostic_entity_res_arena_size];
    struct nk_arena res_arena = NK_ARENA_INITIALIZER(res_buffer, res_buffer + sizeof(res_buffer));

    fprintf(stderr, "[HardwareDiag ] OK\n");

    return EXIT_SUCCESS;
}