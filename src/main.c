#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <string.h>

#include "shared_payload.h"

LOG_MODULE_REGISTER(edge_aggregator, LOG_LEVEL_INF);

K_MSGQ_DEFINE(env_msgq, sizeof(struct env_payload), 10, 4);

#define BME280_NODE DT_NODELABEL(bme280)

void    sensor_thread_fn(void *arg1, void *arg2, void *arg3)
{
    const struct device *dev = DEVICE_DT_GET(BME280_NODE);
    if (!device_is_ready(dev))
    {
        LOG_ERR("BME280 device not ready!");
        return ;
    }

    struct env_payload  payload;
    struct sensor_value temp, press, hum;

    while (1)
    {
        if (sensor_sample_fetch(dev) == 0)
        {
            sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
            sensor_channel_get(dev, SENSOR_CHAN_PRESS, &press);
            sensor_channel_get(dev, SENSOR_CHAN_HUMIDITY, &hum);
            memset(&payload, 0, sizeof(payload));
            payload.timestamp = k_uptime_get_32();
            payload.temperature.val1 = temp.val1;
            payload.temperature.val2 = temp.val2;
            payload.pressure.val1 = press.val1;
            payload.pressure.val2 = press.val2;
            payload.humidity.val1 = hum.val1;
            payload.humidity.val2 = hum.val2;
            
            if (k_msgq_put(&env_msgq, &payload, K_MSEC(100)) != 0)
                LOG_WRN("Message queue full, dropping sample!");
        }
        k_msleep(2000);
    }
}

void    processing_thread_fn(void *arg1, void *arg2, void *arg3)
{
    struct  env_payload data;
    int                 sock = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        LOG_ERR("Failed to create a socket: %d", sock);
        return ;
    }
    struct  sockaddr_in host_addr;
    host_addr.sin_family = AF_INET;
    host_addr.sin_port = htons(8080);
    zsock_inet_pton(AF_INET, "192.168.100.1", &host_addr.sin_addr);

    while (1)
    {
        if (k_msgq_get(&env_msgq, &data, K_FOREVER) == 0)
        {
            int ret = zsock_sendto(sock, &data, sizeof(data), 0, (struct sockaddr *)&host_addr, sizeof(host_addr));
            if (ret < 0)
                LOG_ERR("Socket transmission failed: %d", errno);
            else
                LOG_INF("Transmitted payload at timestamp %u", data.timestamp);
        }
    }
}

K_THREAD_DEFINE(sensor_tid, 1024, sensor_thread_fn, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(processing_tid, 1024, processing_thread_fn, NULL, NULL, NULL, 5, 0, 0);