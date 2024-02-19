#include "diag_msg.h"

char *WriteCommonImpl(__rtl_unused struct traffic_light_IDiagMessage              *self,
                         const traffic_light_IDiagMessage_Write_req               *req,
                         const struct nk_arena                                    *reqArena,
                         __rtl_unused traffic_light_IDiagMessage_Write_res        *res,
                         __rtl_unused struct nk_arena                             *resArena,
                         char                                                     *entityName) {
    nk_uint32_t msgCount = 0;

    nk_ptr_t *message = nk_arena_get(nk_ptr_t, reqArena, &(req->inMessage.message), &msgCount);
    if (message == RTL_NULL) {
        fprintf(stderr, "[%-13s] ERR Can`t get messages from arena!\n", entityName);
        return NK_EBADMSG;
    }

    nk_char_t *msg = RTL_NULL;
    nk_uint32_t msgLen = 0;
    msg = nk_arena_get(nk_char_t, reqArena, &message[0], &msgLen);
    if (msg == RTL_NULL) {
        fprintf(stderr, "[%-13s] ERR Can`t get message from arena!\n", entityName);
        return NK_EBADMSG;
    }

    fprintf(stderr, "[%-13s] GOT [code: %08d, message: %s]\n", entityName, req->inMessage.code, msg);

    return msg;
}
