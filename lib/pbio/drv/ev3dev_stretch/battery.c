/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Laurens Valk
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include <pbio/error.h>
#include <pbio/port.h>

FILE *f_voltage;
FILE *f_current;

void _pbdrv_battery_init(void) {
    f_voltage = fopen("/sys/class/power_supply/lego-ev3-battery/voltage_now", "r");
    f_current = fopen("/sys/class/power_supply/lego-ev3-battery/current_now", "r");
}

void _pbdrv_battery_poll(uint32_t now) { }

#ifdef PBIO_CONFIG_ENABLE_DEINIT
void _pbdrv_battery_deinit(void) {
    if (f_voltage != NULL) {
        fclose(f_voltage);
    }
    if (f_current != NULL) {
        fclose(f_current);
    }
}
#endif

pbio_error_t pbdrv_battery_get_voltage_now(pbio_port_t port, uint16_t *value) {
    int32_t microvolt;
    if (0 == fseek(f_voltage, 0, SEEK_SET) &&
        0 <= fscanf(f_voltage, "%d", &microvolt) &&
        0 == fflush(f_voltage)) {
        *value = (microvolt / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}

pbio_error_t pbdrv_battery_get_current_now(pbio_port_t port, uint16_t *value) {
    int32_t microamp;
    if (0 == fseek(f_current, 0, SEEK_SET) &&
        0 <= fscanf(f_current, "%d", &microamp) &&
        0 == fflush(f_current)) {
        *value = (microamp / 1000);
        return PBIO_SUCCESS;
    }
    return PBIO_ERROR_IO;
}
