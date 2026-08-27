#ifndef SHARED_PAYLOAD_H
#define SHARED_PAYLOAD_H

#include <stdint.h>

struct __attribute__((packed)) sensor_reading
{
    int32_t                 val1;
    int32_t                 val2;
};

struct __attribute__((packed)) env_payload
{
    uint32_t                timestamp;
    struct sensor_reading   temperature;
    struct sensor_reading   pressure;
    struct sensor_reading   humidity;
};

#endif