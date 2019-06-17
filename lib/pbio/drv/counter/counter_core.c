// SPDX-License-Identifier: MIT
// Copyright (c) 2019 David Lechner

#include <pbdrv/config.h>

#if PBDRV_CONFIG_COUNTER

#include <stdbool.h>

#include <pbdrv/counter.h>
#include <pbio/error.h>
#include "sys/process.h"
#include "counter.h"
#include "counter_ev3dev_stretch_iio.h"
#include "counter_stm32f0_gpio_quad_enc.h"

PROCESS(pbdrv_counter_process, "counter driver");

static pbdrv_counter_dev_t *pbdrv_counters[PBDRV_CONFIG_COUNTER_NUM_DEV];

pbio_error_t pbdrv_counter_register(uint8_t id, pbdrv_counter_dev_t *dev) {
    if (id >= PBDRV_CONFIG_COUNTER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (pbdrv_counters[id] != NULL) {
        return PBIO_ERROR_INVALID_OP;
    }

    pbdrv_counters[id] = dev;

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_unregister(pbdrv_counter_dev_t *dev) {
    for (int i = 0; i < PBDRV_CONFIG_COUNTER_NUM_DEV; i++) {
        if (pbdrv_counters[i] == dev) {
            pbdrv_counters[i] = NULL;
            return PBIO_SUCCESS;
        }
    }

    return PBIO_ERROR_INVALID_ARG;
}

pbio_error_t pbdrv_counter_get(uint8_t id, pbdrv_counter_dev_t **dev) {
    if (id >= PBDRV_CONFIG_COUNTER_NUM_DEV) {
        return PBIO_ERROR_INVALID_ARG;
    }

    if (pbdrv_counters[id] == NULL || !pbdrv_counters[id]->initalized) {
        return PBIO_ERROR_AGAIN;
    }

    *dev = pbdrv_counters[id];

    return PBIO_SUCCESS;
}

pbio_error_t pbdrv_counter_get_count(pbdrv_counter_dev_t *dev, int32_t *count) {
    if (!dev->initalized) {
        return PBIO_ERROR_NO_DEV;
    }

    return dev->get_count(dev, count);
}

pbio_error_t pbdrv_counter_get_rate(pbdrv_counter_dev_t *dev, int32_t *rate) {
    if (!dev->initalized) {
        return PBIO_ERROR_NO_DEV;
    }

    return dev->get_rate(dev, rate);
}

static void pbdrv_counter_process_exit() {
#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO
    pbdrv_counter_ev3dev_stretch_iio_drv.exit();
#endif
#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
    pbdrv_counter_stm32f0_gpio_quad_enc_drv.exit();
#endif
}

PROCESS_THREAD(pbdrv_counter_process, ev, data) {
    PROCESS_EXITHANDLER(pbdrv_counter_process_exit());

    PROCESS_BEGIN();

#if PBDRV_CONFIG_COUNTER_EV3DEV_STRETCH_IIO
    pbdrv_counter_ev3dev_stretch_iio_drv.init();
#endif
#if PBDRV_CONFIG_COUNTER_STM32F0_GPIO_QUAD_ENC
    pbdrv_counter_stm32f0_gpio_quad_enc_drv.init();
#endif

    while (true) {
        PROCESS_WAIT_EVENT();
    }

    PROCESS_END();
}

#endif // PBDRV_CONFIG_COUNTER
