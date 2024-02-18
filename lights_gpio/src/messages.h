#ifndef CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_MESSAGES_H

#include <traffic_light/LightsGPIO.edl.h>
#include <traffic_light/IDiagMessage.idl.h>

typedef struct {
    struct traffic_light_IDiagMessage_proxy *proxy;
    traffic_light_IDiagMessage_Write_req *req;
    traffic_light_IDiagMessage_Write_res *res;
    struct nk_arena *reqArena;
    struct nk_arena *resArena;
} TransportDescriptor;

#define DESCR_INIT(proxyIn, reqIn, resIn, reqArenaIn, resArenaIn) { \
            .proxy = proxyIn,                                       \
            .req = reqIn,                                           \
            .res = resIn,                                           \
            .reqArena = reqArenaIn,                                 \
            .resArena = resArenaIn                                  \
        }

void send_diagnostic_message(TransportDescriptor *desc, u_int32_t in_code, char *in_message);

#define CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_MESSAGES_H

#endif //CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_MESSAGES_H
