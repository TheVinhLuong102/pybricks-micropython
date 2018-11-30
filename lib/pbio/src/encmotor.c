/*
 * Copyright (c) 2018 Laurens Valk
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <pbdrv/motor.h>
#include <pbio/dcmotor.h>
#include <pbio/encmotor.h>
#include <inttypes.h>

pbio_error_t pbio_encmotor_setup(pbio_port_t port, pbio_motor_dir_t direction, float_t gear_ratio){

    // Configure DC Motor
    pbio_error_t status = pbio_dcmotor_setup(port, direction);

    //
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    //
    float_t counts_per_unit = 1.0;

    // If all checks have passed, continue with setup of encoded motor
    if (status == PBIO_SUCCESS) {
        encmotor_settings[PORT_TO_IDX(port)].counts_per_unit = counts_per_unit;
        encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit = counts_per_unit * gear_ratio;
        encmotor_settings[PORT_TO_IDX(port)].offset = 0;
        status = pbio_encmotor_reset_encoder_count(port, 0);
    }
    // TODO: Use the device_id to retrieve the default settings defined in our lib. For now just hardcode something below.
    pbio_encmotor_set_settings(port, 100, 2, 500, 5, 1000, 1, 1000, 1000, 100, 800, 800, 5);
    return status;
}

pbio_error_t pbio_encmotor_set_settings(
        pbio_port_t port,
        int16_t stall_torque_limit_pct,
        int32_t stall_speed_limit,
        int16_t stall_time,
        int32_t speed_tolerance,
        int32_t max_speed,
        int32_t position_tolerance,
        int32_t acceleration_start,
        int32_t acceleration_end,
        int32_t tight_loop_time,
        int16_t pid_kp,
        int16_t pid_ki,
        int16_t pid_kd
    ){
    pbio_error_t status = pbio_dcmotor_set_settings(port, stall_torque_limit_pct);
    if (status != PBIO_SUCCESS) {
        return status;
    }
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    encmotor_settings[port_index].stall_rate_limit = (counts_per_output_unit * stall_speed_limit);
    encmotor_settings[port_index].stall_time = stall_time * US_PER_MS;
    encmotor_settings[port_index].rate_tolerance = (counts_per_output_unit * speed_tolerance);
    encmotor_settings[port_index].max_rate = (counts_per_output_unit * max_speed);
    encmotor_settings[port_index].count_tolerance = (counts_per_output_unit * position_tolerance);
    encmotor_settings[port_index].abs_accl_start = (counts_per_output_unit * acceleration_start);
    encmotor_settings[port_index].abs_accl_end = (counts_per_output_unit * acceleration_end);
    encmotor_settings[port_index].tight_loop_time = tight_loop_time * US_PER_MS;
    encmotor_settings[port_index].pid_kp = pid_kp;
    encmotor_settings[port_index].pid_ki = pid_ki;
    encmotor_settings[port_index].pid_kd = pid_kd;
    return PBIO_SUCCESS;
};

void pbio_encmotor_print_settings(pbio_port_t port, char *settings_string){
    // Preload several settings for easier printing
    int8_t port_index = PORT_TO_IDX(port);
    float_t counts_per_output_unit = encmotor_settings[port_index].counts_per_output_unit;
    float_t counts_per_unit = encmotor_settings[port_index].counts_per_unit;
    float_t gear_ratio = counts_per_output_unit / encmotor_settings[port_index].counts_per_unit;
    // Print settings to settings_string
    snprintf(settings_string, MAX_ENCMOTOR_SETTINGS_STR_LENGTH,
        "Counts per unit\t %" PRId32 ".%" PRId32 "\n"
        "Gear ratio\t %" PRId32 ".%" PRId32 "\n"
        "Stall speed\t %" PRId32 "\n"
        "Stall time\t %" PRId32 "\n"
        "Speed tolerance\t %" PRId32 "\n"
        "Max speed\t %" PRId32 "\n"
        "Angle tolerance\t %" PRId32 "\n"
        "Acceleration\t %" PRId32 "\n"
        "Deceleration\t %" PRId32 "\n"
        "Tight Loop\t %" PRId32 "\n"
        "kp\t\t %" PRId32 "\n"
        "ki\t\t %" PRId32 "\n"
        "kd\t\t %" PRId32 "",
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (counts_per_unit),
        (int32_t) (counts_per_unit*1000 - ((int32_t) counts_per_unit)*1000),
        // Print counts_per_unit as floating point with 3 decimals
        (int32_t) (gear_ratio),
        (int32_t) (gear_ratio*1000 - ((int32_t) gear_ratio)*1000),
        // Print remaining settings as integers
        (int32_t) (encmotor_settings[port_index].stall_rate_limit / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].stall_time  / US_PER_MS),
        (int32_t) (encmotor_settings[port_index].rate_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].max_rate / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].count_tolerance / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].abs_accl_start / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].abs_accl_end / counts_per_output_unit),
        (int32_t) (encmotor_settings[port_index].tight_loop_time / US_PER_MS),
        (int32_t) encmotor_settings[port_index].pid_kp,
        (int32_t) encmotor_settings[port_index].pid_ki,
        (int32_t) encmotor_settings[port_index].pid_kd
    );
}

bool pbio_encmotor_has_encoder(pbio_port_t port){
    int32_t count;
    return pbdrv_motor_get_encoder_count(port, &count) == PBIO_SUCCESS;
}

pbio_error_t pbio_encmotor_get_encoder_count(pbio_port_t port, int32_t *count) {
    pbio_error_t status = pbdrv_motor_get_encoder_count(port, count);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *count = -*count;
    }
    *count -= encmotor_settings[PORT_TO_IDX(port)].offset;
    return status;
}

pbio_error_t pbio_encmotor_reset_encoder_count(pbio_port_t port, int32_t reset_count) {
    // Set the motor to coast and stop any running maneuvers
    pbio_dcmotor_coast(port);

    // First get the counter value without any offsets, but with the appropriate polarity/sign.
    int32_t count_no_offset;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &count_no_offset);
    count_no_offset += encmotor_settings[PORT_TO_IDX(port)].offset;

    // Calculate the new offset
    encmotor_settings[PORT_TO_IDX(port)].offset = count_no_offset - reset_count;

    return status;
}

pbio_error_t pbio_encmotor_get_angle(pbio_port_t port, int32_t *angle) {
    int32_t encoder_count;
    pbio_error_t status = pbio_encmotor_get_encoder_count(port, &encoder_count);
    *angle = encoder_count / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;
}

pbio_error_t pbio_encmotor_reset_angle(pbio_port_t port, int32_t reset_angle) {
    return pbio_encmotor_reset_encoder_count(port, (int32_t) (reset_angle * encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit));
}

pbio_error_t pbio_encmotor_get_encoder_rate(pbio_port_t port, int32_t *rate) {
    pbio_error_t status = pbdrv_motor_get_encoder_rate(port, rate);
    if (dcmotor_settings[PORT_TO_IDX(port)].direction == PBIO_MOTOR_DIR_INVERTED) {
        *rate = -*rate;
    }
    return status;
}

pbio_error_t pbio_encmotor_get_angular_rate(pbio_port_t port, int32_t *angular_rate) {
    int32_t encoder_rate;
    pbio_error_t status = pbio_encmotor_get_encoder_rate(port, &encoder_rate);
    *angular_rate = encoder_rate / (encmotor_settings[PORT_TO_IDX(port)].counts_per_output_unit);
    return status;
}
