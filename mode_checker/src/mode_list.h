#ifndef ___MODE_LIST___
/* Valid lights combinations */
static const rtl_uint32_t TRAFFIC_LIGHT_MODES[] = {
        traffic_light_IMode_R1,
        traffic_light_IMode_R1 + traffic_light_IMode_AR1,
        traffic_light_IMode_R1 + traffic_light_IMode_Y1,
        traffic_light_IMode_R1 + traffic_light_IMode_Y1 + traffic_light_IMode_AR1,
        traffic_light_IMode_G1,
        traffic_light_IMode_G1 + traffic_light_IMode_AL1 + traffic_light_IMode_AR1,
        traffic_light_IMode_G1 + traffic_light_IMode_AR1
};
#define ___MODE_LIST___
#endif