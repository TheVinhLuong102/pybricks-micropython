// SPDX-License-Identifier: MIT
// Copyright (c) 2018-2020 Laurens Valk
// Copyright (c) 2020 LEGO System A/S

#include <pbio/drivebase.h>
#include <pbio/motorpoll.h>
#include <pbio/servo.h>

#if PBDRV_CONFIG_NUM_MOTOR_CONTROLLER != 0

static pbio_servo_t servo[PBDRV_CONFIG_NUM_MOTOR_CONTROLLER];
static pbio_drivebase_t drivebase;

pbio_error_t pbio_motorpoll_get_servo(pbio_port_t port, pbio_servo_t **srv) {
    // Validate port
    if (port < PBDRV_CONFIG_FIRST_MOTOR_PORT || port > PBDRV_CONFIG_LAST_MOTOR_PORT) {
        return PBIO_ERROR_INVALID_PORT;
    }
    // Get pointer to servo object
    *srv = &servo[port - PBDRV_CONFIG_FIRST_MOTOR_PORT];
    (*srv)->port = port;
    return PBIO_SUCCESS;
}

pbio_error_t pbio_motorpoll_get_drivebase(pbio_drivebase_t **db) {
    // Get pointer to device (at the moment, there is only one drivebase)
    *db = &drivebase;
    return PBIO_SUCCESS;
}


void _pbio_motorpoll_reset_all(void) {
    int i;
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_setup(&servo[i], PBIO_DIRECTION_CLOCKWISE, fix16_one);
    }
}

void _pbio_motorpoll_poll(void) {
    int i;
    // Do the update for each motor
    for (i = 0; i < PBDRV_CONFIG_NUM_MOTOR_CONTROLLER; i++) {
        pbio_servo_t *srv = &servo[i];

        // FIXME: Use a better solution skip servicing disconnected connected servos.
        if (!srv->connected) {
            continue;
        }
        srv->connected = pbio_servo_control_update(srv) == PBIO_SUCCESS;
    }

    pbio_drivebase_t *db = &drivebase;

    if (db->left && db->left->connected && db->right && db->right->connected) {
        pbio_drivebase_update(db);
    }
}

#endif // PBDRV_CONFIG_NUM_MOTOR_CONTROLLER