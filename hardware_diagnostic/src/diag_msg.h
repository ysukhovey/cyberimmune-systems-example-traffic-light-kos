#ifndef CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_DIAG_MSG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <rtl/string.h>

#include <traffic_light/HardwareDiagnostic.edl.h>
#include <traffic_light/IDiagMessage.idl.h>


char* WriteCommonImpl(__rtl_unused struct traffic_light_IDiagMessage              *self,
                         const traffic_light_IDiagMessage_Write_req               *req,
                         const struct nk_arena                                    *reqArena,
                         __rtl_unused traffic_light_IDiagMessage_Write_res        *res,
                         __rtl_unused struct nk_arena                             *resArena,
                         char                                                     *entityName);

struct traffic_light_IDiagMessage *CreateIDiagMessageImpl(void);

#define CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_DIAG_MSG_H

#endif //CYBERIMMUNE_SYSTEMS_EXAMPLE_TRAFFIC_LIGHT_KOS_DIAG_MSG_H
