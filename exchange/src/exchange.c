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
#include <strings.h>

#include <coresrv/nk/transport-kos.h>
#include <coresrv/sl/sl_api.h>

#include <traffic_light/Exchange.edl.h>
#include <traffic_light/IMode.idl.h>
#include <traffic_light/IDiagMessage.idl.h>

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

void init_http_server_connection() {

}

uint32_t request_data_from_http_server() {
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "[Exchange     ] DEBUG: Socket creation failed...\n\n\n");
        return EXIT_FAILURE;
    }
    else
        fprintf(stderr, "[Exchange     ] DEBUG: Socket successfully created..\n\n\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(HOST_IP);

    if ( servaddr.sin_addr.s_addr == INADDR_NONE ) {
        fprintf(stderr, "[Exchange     ] bad address!");
    }

    servaddr.sin_port = htons(HOST_PORT);

    int res = -1;

    res = -1;
    for (int i = 0; res == -1 && i < NUM_RETRIES; i++) {
        sleep(1); // Wait some time for server be ready.
        res = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    }

    if (res != 0)
        fprintf(stderr, "[Exchange     ] DEBUG: Connection with the server failed... %d\n", res);
    else
        fprintf(stderr, "[Exchange     ] DEBUG: Connected to the server\n");

    char request_data[MSG_BUF_SIZE + 1];
    char response_data[MSG_BUF_SIZE + 1];
    int  request_len = 0;
    size_t n;

    snprintf(request_data, MSG_CHUNK_BUF_SIZE,
             "GET %s HTTP/1.1\r\n"
             "Host: %s:%d\r\n\r\n"
            // "Host-Agent: KOS\r\n"
            // "Accept: */*\r\n"
            , HOST_PATH, HOST_IP, HOST_PORT);

    //fprintf(stderr, "[Exchange     ] DEBUG: Request prepared:\n%s\n", request_data);

    request_len = strlen(request_data);
    write(sockfd, request_data, request_len);
    write(sockfd, request_data, request_len);

    if (write(sockfd, request_data, request_len)>= 0) {
        fprintf(stderr, "[Exchange     ] Request sent, reading response..\n");
        while ((n = read(sockfd, response_data, MSG_BUF_SIZE)) > 0) {
            response_data[n] = '\0';
            fprintf(stderr, "[Exchange     ] response data: \n%s\n\n", response_data);
        }
    }

    close(sockfd);

    return EXIT_SUCCESS;
}

uint32_t send_message_to_control_system(traffic_light_IDiagMessage_DiagnosticMessage message) {
    sprintf(stderr, "[Exchange     ] code: %08x, message: %p\n", message.code, message.message);
    return 0;
}

static nk_err_t WriteImpl(__rtl_unused struct traffic_light_IDiagMessage    *self,
                          const traffic_light_IDiagMessage_Write_req        *req,
                          const struct nk_arena                             *reqArena,
                          __rtl_unused traffic_light_IDiagMessage_Write_res *res,
                          __rtl_unused struct nk_arena                      *resArena) {
    nk_uint32_t msgCount = 0;

    nk_ptr_t *message = nk_arena_get(nk_ptr_t, reqArena, &(req->inMessage.message), &msgCount);
    if (message == RTL_NULL) {
        fprintf(stderr, "[Exchange     ] Error: can`t get messages from arena!\n");
        return NK_EBADMSG;
    }

    nk_char_t *msg = RTL_NULL;
    nk_uint32_t msgLen = 0;
    msg = nk_arena_get(nk_char_t, reqArena, &message[0], &msgLen);
    if (msg == RTL_NULL) {
        fprintf(stderr, "[Exchange     ] Error: can`t get message from arena!\n");
        return NK_EBADMSG;
    }

    fprintf(stderr, "[Exchange     ] GOT %08d: %s\n", req->inMessage.code, msg);

    return NK_EOK;
}

static struct traffic_light_IDiagMessage *CreateIDiagMessageImpl(void) {
    static const struct traffic_light_IDiagMessage_ops Ops = {
            .Write = WriteImpl
    };
    static traffic_light_IDiagMessage obj = {
            .ops = &Ops
    };
    return &obj;
}

int main(int argc, const char *argv[]) {
    int               socketfd;
    struct            ifconf conf;
    struct            ifreq iface_req[DISCOVERING_IFACE_MAX];
    struct            ifreq *ifr;
    struct sockaddr * sa;
    bool              is_network_available;

    fprintf(stderr, "[Exchange     ] Hello, I'm about to start working\n");

    is_network_available = wait_for_network();

    fprintf(stderr, "[Exchange     ] Network status: %s\n", is_network_available ? "OK" : "FAIL");
    fprintf(stderr, "[Exchange     ] Opening socket...\n");

    socketfd = socket(AF_ROUTE, SOCK_RAW, 0);
    if (socketfd < 0) {
        fprintf(stderr, "\n[Exchange     ] cannot create socket\n");
        return EXIT_FAILURE;
    }

    conf.ifc_len = sizeof(iface_req);
    conf.ifc_buf = (__caddr_t) iface_req;

    if (ioctl(socketfd, SIOCGIFCONF, &conf) < 0) {
        fprintf(stderr, "[Exchange     ] ioctl call failed\n");
        close(socketfd);
        return EXIT_FAILURE;
    }

    fprintf(stderr, "[Exchange     ] Discovering interfaces...\n");

    for (size_t i = 0; i < conf.ifc_len / sizeof(iface_req[0]); i ++) {
        ifr = &conf.ifc_req[i];
        sa = (struct sockaddr *) &ifr->ifr_addr;

        if (sa->sa_family == AF_INET) {
            struct sockaddr_in *sin = (struct sockaddr_in*) &ifr->ifr_addr;

            fprintf(stderr, "[Exchange     ] %s %s\n", ifr->ifr_name, inet_ntoa(sin->sin_addr));
        }
    }

    fprintf(stderr, "[Exchange     ] Network check up: OK\n");

    int config = request_data_from_http_server();

    fprintf(stderr, "[Exchange     ] OK (%08x)\n", config);

    for(;;) {
        request_data_from_http_server();

    /*    traffic_light_IDiagMessage_DiagnosticMessage message = {
                .code = 0x77777777,
                .message = NK_NULL
        };
        send_message_to_control_system(message);
*/
        // todo 1. request messages from the http-server
        // todo 1.1. send mode to the ControlSystem
        // todo 2. receive (if any) status message from the ControlSystem
    }

    return EXIT_SUCCESS;
}