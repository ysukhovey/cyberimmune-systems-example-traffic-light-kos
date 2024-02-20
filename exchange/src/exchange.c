#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <kos_net.h>
#include <string.h>
#include <strings.h>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#include <traffic_light/Exchange.edl.h>
#include <traffic_light/ICode.idl.h>
#include <traffic_light/IMode.idl.h>

#include "../../microjson/src/mjson.h"

#include <assert.h>

#define DISCOVERING_IFACE_MAX   10
#define TIME_STEP_SEC           5
#define HOST_IP                 "172.16.0.100"
#define HOST_PORT               3000
#define HOST_PATH               "/traffic_light"
#define NUM_RETRIES             10
#define MSG_BUF_SIZE            1024
#define MSG_CHUNK_BUF_SIZE      256
#define SA struct sockaddr

#define EXAMPLE_VALUE_TO_SEND 777

#define ARR_LENGTH 128

static uint32_t combinations[ARR_LENGTH];
static int c_count;

const struct json_attr_t json_combinations[] = {
        {"combinations", t_array,
         .addr.array.element_type = t_uinteger,
         .addr.array.arr.uintegers = combinations,
         .addr.array.maxlen = ARR_LENGTH,
         .addr.array.count = &c_count},
        {NULL},
};

void init_http_server_connection() {

}

uint32_t send_message_to_control_system(traffic_light_IDiagMessage_DiagnosticMessage message) {
    sprintf(stderr, "[Exchange     ] code: %08x, message: %p\n", message.code, message.message);
    return 0;
}

uint32_t request_data_from_http_server() {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "[Exchange     ] Socket creation failed.\n");
        return EXIT_FAILURE;
    }
    else
        fprintf(stderr, "[Exchange     ] Socket successfully created.\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(HOST_IP);

    if ( servaddr.sin_addr.s_addr == INADDR_NONE ) {
        fprintf(stderr, "[Exchange     ] Bad address!");
    }

    servaddr.sin_port = htons(HOST_PORT);

    int res = -1;

    res = -1;
    for (int i = 0; res == -1 && i < NUM_RETRIES; i++) {
        sleep(1); // Wait some time for server be ready.
        res = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    }

    if (res != 0) {
        fprintf(stderr, "[Exchange     ] Connection with the server failed (%d)\n", res);
    } else {
        fprintf(stderr, "[Exchange     ] Connected to the server\n");

        char request_data[MSG_BUF_SIZE + 1];
        char response_data[MSG_BUF_SIZE + 1];
        int request_len = 0;
        size_t n;

        snprintf(request_data, MSG_CHUNK_BUF_SIZE,
                 "GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n", HOST_PATH, HOST_IP, HOST_PORT);

        //fprintf(stderr, "[Exchange     ] Request prepared:\n%s\n", request_data);
        request_len = strlen(request_data);
        write(sockfd, request_data, request_len);
        if (write(sockfd, request_data, request_len) >= 0) {
            fprintf(stderr, "[Exchange     ] Request sent, reading response.\n");
            while ((n = read(sockfd, response_data, MSG_BUF_SIZE)) > 0) {
                response_data[n] = '\0';
                char *json_start = strstr(response_data, "\"{") + 1;
                char *json_end = strstr(json_start, "}");
                *(json_end + 1) = '\0';
                fprintf(stderr, "[Exchange     ] Found JSON: %s\n", json_start);
                c_count = 0;
                int status = json_read_object(json_start, json_combinations, NULL);
                for (int i = 0; i < c_count; i++)
                    fprintf(stderr, "[Exchange     ] Found combination [%08x]\n", combinations[i]);
                if (status != 0)
                    fprintf(stderr, "[Exchange     ] ERR JSON Parsing error: %s\n", json_error_string(status));

                //fprintf(stderr, "[Exchange     ] response data: \n%s\n\n", response_data);
                // todo here the sending to the ControlSystem should start
                // todo extract JSON, parse it and send combination(s)
                //send_message_to_control_system(message);

            }

        }
    }
    close(sockfd);

    return EXIT_SUCCESS;
}

typedef struct ICodeImpl {
    struct traffic_light_ICode base;
} ICodeImpl;

static nk_err_t FCode_impl(struct traffic_light_ICode                   *self,
                           const struct traffic_light_ICode_FCode_req   *req,
                           const struct nk_arena                        *req_arena,
                           traffic_light_ICode_FCode_res                *res,
                           struct nk_arena                              *res_arena) {
    res->result = req->value;
    return NK_EOK;
}

static struct traffic_light_ICode *CreateICodeImpl() {
    static const struct traffic_light_ICode_ops ops = {
            .FCode = FCode_impl
    };
    static struct ICodeImpl impl = {
            .base = {&ops}
    };
    return &impl.base;
}

int main(int argc, const char *argv[]) {
    int               socketfd;
    struct            ifconf conf;
    struct            ifreq iface_req[DISCOVERING_IFACE_MAX];
    struct            ifreq *ifr;
    struct sockaddr * sa;
    bool              is_network_available;

    is_network_available = wait_for_network();

    fprintf(stderr, "[Exchange     ] INF Network status: %s\n", is_network_available ? "OK" : "FAIL");
    fprintf(stderr, "[Exchange     ] INF Opening socket...\n");

    socketfd = socket(AF_ROUTE, SOCK_RAW, 0);
    if (socketfd < 0) {
        fprintf(stderr, "\n[Exchange     ] ERR Cannot create socket\n");
        return EXIT_FAILURE;
    }

    conf.ifc_len = sizeof(iface_req);
    conf.ifc_buf = (__caddr_t) iface_req;

    if (ioctl(socketfd, SIOCGIFCONF, &conf) < 0) {
        fprintf(stderr, "[Exchange     ] ERR ioctl call failed\n");
        close(socketfd);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[Exchange     ] INF Discovering interfaces...\n");

    for (size_t i = 0; i < conf.ifc_len / sizeof(iface_req[0]); i ++) {
        ifr = &conf.ifc_req[i];
        sa = (struct sockaddr *) &ifr->ifr_addr;
        if (sa->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in*) &ifr->ifr_addr;
            fprintf(stderr, "[Exchange     ] INF %s %s\n", ifr->ifr_name, inet_ntoa(sin->sin_addr));
        }
    }

    fprintf(stderr, "[Exchange     ] INF Network check up OK\n");

    int config = request_data_from_http_server();

    //
    NkKosTransport cse_transport;
    ServiceId cse_iid;
    Handle cse_handle = ServiceLocatorRegister("cs_exchange_connection", NULL, 0, &cse_iid);
    assert(cse_handle != INVALID_HANDLE);
    NkKosTransport_Init(&cse_transport, cse_handle, NK_NULL, 0);
    traffic_light_ICode_FCode_req cse_req;
    char cse_req_buffer[traffic_light_ICode_FCode_req_arena_size];
    struct nk_arena cse_req_arena = NK_ARENA_INITIALIZER(cse_req_buffer, cse_req_buffer + sizeof(cse_req_buffer));
    traffic_light_ICode_FCode_res cse_res;
    char cse_res_buffer[traffic_light_ICode_FCode_res_arena_size];
    struct nk_arena cse_res_arena = NK_ARENA_INITIALIZER(cse_res_buffer, cse_res_buffer + sizeof(cse_res_buffer));
    fprintf(stderr, "[Exchange     ] ControlSystem service transport register OK\n");

    // ControlCenter transport infrastructure
    NkKosTransport cs_transport;
    struct traffic_light_IMode_proxy cs_proxy;
    Handle cs_handle = ServiceLocatorConnect("exchange_cs_connection");
    assert(cs_handle != INVALID_HANDLE);
    NkKosTransport_Init(&cs_transport, cs_handle, NK_NULL, 0);
    //nk_iid_t cs_riid = ServiceLocatorGetRiid(cs_handle, "traffic_light.ControlSystem.write");
    nk_iid_t cs_riid = ServiceLocatorGetRiid(cs_handle, "traffic_light.ControlSystem.mode");
    assert(cs_riid != INVALID_RIID);
    traffic_light_IMode_proxy_init(&cs_proxy, &cs_transport.base, cs_riid);
    traffic_light_IMode_FMode_req cs_req;
    traffic_light_IMode_FMode_res cs_res;
    fprintf(stderr, "[Exchange     ] ControlSystem client transport location OK\n");

    fprintf(stderr, "[Exchange     ] OK\n");

    for(;;) {
        // todo 1. request messages from the http-server
        request_data_from_http_server();
        // todo 2. receive (if any) status message from the ControlSystem
        cs_req.value = 0x41414141;
        uint32_t sendingResult = traffic_light_IMode_FMode(&cs_proxy.base, &cs_req, NULL, &cs_res, NULL);
        if (sendingResult == rcOk) {
            //traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
        } else {
            fprintf(stderr, "[Exchange     ] ERR Failed to call ControlCenter.IMode.FMode() [%d]\n", sendingResult);
/*
            traffic_light_ModeChecker_entity_dispatch(&entity, &req.base_, &req_arena, &res.base_, &res_arena);
            res.modeChecker_mode.FMode.result = traffic_light_IMode_WRONGCOMBO;
*/
        }
    }

    return EXIT_SUCCESS;
}