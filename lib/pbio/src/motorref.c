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

#include <pbio/motorref.h>
#include <stdlib.h>
#include <math.h>

#include "sys/clock.h"

void reverse_trajectory(pbio_motor_trajectory_t *ref) {
    // Mirror angles about initial angle th0
    ref->th1 = 2*ref->th0 - ref->th1;
    ref->th2 = 2*ref->th0 - ref->th2;
    ref->th3 = 2*ref->th0 - ref->th3;

    // Negate speeds and accelerations
    ref->w0 *= -1;
    ref->w1 *= -1;
    ref->a0 *= -1;
    ref->a2 *= -1;
}

void make_trajectory_none(ustime_t t0, count_t th0, rate_t w1, pbio_motor_trajectory_t *ref) {
    // All times equal to initial time:
    ref->t0 = t0;
    ref->t1 = t0;
    ref->t2 = t0;
    ref->t3 = t0;

    // All angles equal to initial angle:
    ref->th0 = th0;
    ref->th1 = th0;
    ref->th2 = th0;
    ref->th3 = th0;

    // All speeds/accelerations zero:
    ref->w0 = 0;
    ref->w1 = w1;
    ref->a0 = 0;
    ref->a2 = 0;
}

pbio_error_t make_trajectory_time_based(ustime_t t0, ustime_t t3, count_t th0, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_motor_trajectory_t *ref) {

    // Work with time intervals instead of absolute time. Read 'm' as '-'.
    ustime_t t3mt0 = t3-t0;
    ustime_t t3mt2;
    ustime_t t2mt1;
    ustime_t t1mt0;

    // Return error for negative user-specified duration
    if (t3mt0 < 0) {
        return PBIO_ERROR_INVALID_ARG;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = wt < 0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        wt *= -1;
        w0 *= -1;
    }

    // Limit initial speed
    rate_t max_init = timest(a, t3mt0);
    rate_t abs_max = min(wmax, max_init);
    w0 = max(-abs_max, min(w0, abs_max));

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate
        ref->a0 = a;
        // If target speed can be reached
        if (wdiva(wt-w0, a) - (t3mt0-wdiva(w0, a))/2 < 0) {
            t1mt0 = wdiva(wt-w0, a);
            ref->w1 = wt;
        }
        // If target speed cannot be reached
        else {
            t1mt0 = (t3mt0-wdiva(w0, a))/2;
            ref->w1 = timest(a, t3mt0)/2+w0/2;
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate
        ref->a0 = -a;
        t1mt0 = wdiva(w0-wt, a);
        ref->w1 = wt;
    }

    // # Deceleration phase
    ref->a2 = -a;
    t3mt2 = wdiva(ref->w1, a);

    // Constant speed duration
    t2mt1 = t3mt0 - t3mt2 - t1mt0;

    // Assert that all time intervals are positive
    if (t1mt0 < 0 || t2mt1 < 0 || t3mt2 < 0) {
        return PBIO_ERROR_FAILED;
    }

    // Store other results/arguments
    ref->w0 = w0;
    ref->th0 = th0;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = t0 + t1mt0 + t2mt1;
    ref->t3 = t3;

    // Corresponding angle values
    ref->th1 = ref->th0 + timest(ref->w0, t1mt0) + timest2(ref->a0, t1mt0);
    ref->th2 = ref->th1 + timest(ref->w1, t2mt1);
    ref->th3 = ref->th2 + timest(ref->w1, t3mt2) + timest2(ref->a2, t3mt2);

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }

    return PBIO_SUCCESS;
}


pbio_error_t make_trajectory_angle_based(ustime_t t0, count_t th0, count_t th3, rate_t w0, rate_t wt, rate_t wmax, accl_t a, pbio_motor_trajectory_t *ref) {

    // Return error for zero speed
    if (wt == 0) {
        return PBIO_ERROR_INVALID_ARG;
    }
    // Return empty maneuver for zero angle
    if (th3 == th0) {
        make_trajectory_none(t0, th0, 0, ref);
        return PBIO_SUCCESS;
    }

    // Remember if the original user-specified maneuver was backward
    bool backward = th3 < th0;

    // Convert user parameters into a forward maneuver to simplify computations (we negate results at the end)
    if (backward) {
        th3 = 2*th0 - th3;
        w0 *= -1;
    }
    // In a forward maneuver, the target speed is always positive.
    wt = abs(wt);

    // Limit initial speed, but evaluate square root only if necessary (usually not)
    if (w0 > 0 && (w0*w0)/(2*a) > th3 - th0) {
        w0 = sqrtf(2*a*(th3 - th0)); // TODO: Use int implementation
    }

    // Initial speed is less than the target speed
    if (w0 < wt) {
        // Therefore accelerate towards intersection from below,
        // either by reaching constant speed phase or not.
        ref->a0 = a;

        // Fictitious zero speed angle (ahead of us if we have negative initial speed; behind us if we have initial positive speed)
        count_t thf = th0 - (w0*w0)/(2*a);

        // Test if we can get to ref speed
        if (th3-thf >= (wt*wt)/a) {
            //  If so, find both constant speed intersections
            ref->th1 = thf + (wt*wt)/(2*a);
            ref->th2 = th3 - (wt*wt)/(2*a);
            ref->w1 = wt;
        }
        else {
            // Otherwise, intersect halfway between accelerating and decelerating square root arcs
            ref->th1 = (th3+thf)/2;
            ref->th2 = ref->th1;
            ref->w1 = sqrtf(2*a*(ref->th1-thf)); // TODO: Use int implementation
        }
    }
    // Initial speed is equal to or more than the target speed
    else {
        // Therefore decelerate towards intersection from above
        ref->a0 = -a;
        ref->th1 = th0 + (w0*w0-wt*wt)/(2*a);
        ref->th2 = th3 - (wt*wt)/(2*a);
        ref->w1 = wt;
    }
    // Corresponding time intervals
    ustime_t t1mt0 = wdiva(ref->w1-w0, ref->a0);
    ustime_t t2mt1 = ref->th2 == ref->th1 ? 0 : wdiva(ref->th2-ref->th1, ref->w1);
    ustime_t t3mt2 = wdiva(ref->w1, a);

    // Store other results/arguments
    ref->w0 = w0;
    ref->th0 = th0;
    ref->th3 = th3;
    ref->t0 = t0;
    ref->t1 = t0 + t1mt0;
    ref->t2 = ref->t1 + t2mt1;
    ref->t3 = ref->t2 + t3mt2;
    ref->a2 = -a;

    // Reverse the maneuver if the original arguments imposed backward motion
    if (backward) {
        reverse_trajectory(ref);
    }
    return PBIO_SUCCESS;
}

// Calculate the characteristic time values, encoder values, rate values and accelerations that uniquely define the rate and count trajectories
pbio_error_t make_motor_trajectory(pbio_port_t port,
                                   pbio_motor_action_t action,
                                   int32_t speed_target,
                                   int32_t duration_or_target_position,
                                   pbio_motor_after_stop_t after_stop){

    // Read the current system state for this motor
    ustime_t time_start = clock_usecs();
    count_t count_start;
    rate_t rate_start;
    pbio_error_t err;

    pbio_motor_trajectory_t *traject = &trajectories[PORT_TO_IDX(port)];
    pbio_encmotor_settings_t *settings = &encmotor_settings[PORT_TO_IDX(port)];

    ustime_t previous_time_start = traject->t0;
    pbio_motor_action_t previous_action = traject->action;

    bool currently_active =
        motor_control_active[PORT_TO_IDX(port)] == PBIO_MOTOR_CONTROL_RUNNING ||
        motor_control_active[PORT_TO_IDX(port)] == PBIO_MOTOR_CONTROL_HOLDING;

    // Experimental work around for run in fast loop
    if (action == RUN &&
        previous_action == RUN &&
        (currently_active || motor_control_active[PORT_TO_IDX(port)] >= PBIO_MOTOR_CONTROL_STARTING) &&
        time_start - previous_time_start < settings->tight_loop_time) {
        get_reference(time_start, traject, &count_start, &rate_start);
        make_trajectory_none(time_start, count_start, speed_target*settings->counts_per_output_unit, traject);
        return PBIO_SUCCESS;
    }

    // Handle track target
    if (action == TRACK_TARGET) {
        // If the previous action was also track target, just keep running, otherwise start a new maneuver
        motor_control_active[PORT_TO_IDX(port)] = previous_action == TRACK_TARGET && currently_active ?
                                                  PBIO_MOTOR_CONTROL_RUNNING:
                                                  PBIO_MOTOR_CONTROL_STARTING;
        traject->action = action;
        make_trajectory_none(time_start, duration_or_target_position*settings->counts_per_output_unit, 0, traject);
        return PBIO_SUCCESS;
    }

    // For now, disable smooth transitions if the next step is a position based maneuver
    // TODO: implement the actual transtion properly
    if (action == RUN_TARGET || action == RUN_ANGLE) {
        currently_active = 0;
    }

    if (currently_active) {
        // Continue the reference from the currently active maneuver, for a smooth transition

        // TODO: FOR RUN_ANGLE, RUN_TIME this isn't the absolute time: subtract paused time

        // Related, use time_stopped to maintain run maneuvers.

        get_reference(time_start, traject, &count_start, &rate_start);
    }
    else {
        // Otherwise, start the reference from the current position and speed
        err = pbio_encmotor_get_encoder_count(port, &count_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
        err = pbio_encmotor_get_encoder_rate(port, &rate_start);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }

    traject->action = action;
    traject->after_stop = after_stop;

    if (action == RUN_TIME || action == RUN || action == RUN_STALLED) {
        if (action != RUN_TIME) {
            // For RUN and RUN_STALLED, no end time is specified, so we take a
            // fictitious 30 seconds. This allows us to use the same code to get
            // the acceleration parameters. Later on, when computing the actual
            // references in the control loop, we skip the deceleration
            // phase, such that RUN and RUN_STALLED are indeed infinite.
            duration_or_target_position = 30*1000;
        }
        err = make_trajectory_time_based(
            time_start,
            time_start + duration_or_target_position*US_PER_MS,
            count_start,
            rate_start,
            speed_target*settings->counts_per_output_unit,
            settings->max_rate,
            settings->abs_acceleration,
            traject);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else if (action == RUN_ANGLE || action == RUN_TARGET) {

        // The absolute target count
        count_t count_target;

        if (action == RUN_TARGET) {
            // In target mode, the user argument is just the target
            count_target = duration_or_target_position*settings->counts_per_output_unit;
        }
        else {
            // If the speed is negative, the meaning of angle reverses
            int32_t angle_relative = speed_target > 0 ? duration_or_target_position: -duration_or_target_position;

            // The absolute is then the starting point plus the relative angle
            count_target = angle_relative*settings->counts_per_output_unit + count_start;
        }
        err = make_trajectory_angle_based(
            time_start,
            count_start,
            count_target,
            rate_start,
            speed_target*settings->counts_per_output_unit,
            settings->max_rate,
            settings->abs_acceleration,
            traject);
        if (err != PBIO_SUCCESS) {
            return err;
        }
    }
    else {
        // ?
    }

    // If this new maneuver aborts an existing running or holding command, we "restart"
    // while keeping PID status. Otherwise, we just "start" with zero PID parameters.
    motor_control_active[PORT_TO_IDX(port)] = currently_active ?
        PBIO_MOTOR_CONTROL_RESTARTING:
        PBIO_MOTOR_CONTROL_STARTING;

    return PBIO_SUCCESS;
}

// Evaluate the reference speed and velocity at the (shifted) time
void get_reference(ustime_t time_ref, pbio_motor_trajectory_t *traject, count_t *count_ref, rate_t *rate_ref){
    // For RUN and RUN_STALLED, the end time is infinite, meaning that the reference signals do not have a deceleration phase
    if (time_ref - traject->t1 < 0) {
        // If we are here, then we are still in the acceleration phase. Includes conversion from microseconds to seconds, in two steps to avoid overflows and round off errors
        *rate_ref = traject->w0   + timest(traject->a0, time_ref-traject->t0);
        *count_ref = traject->th0 + timest(traject->w0, time_ref-traject->t0) + timest2(traject->a0, time_ref-traject->t0);
    }
    else if ((traject->action == RUN) || (traject->action == RUN_STALLED) || time_ref - traject->t2 <= 0) {
        // If we are here, then we are in the constant speed phase
        *rate_ref = traject->w1;
        *count_ref = traject->th1 + timest(traject->w1, time_ref-traject->t1);
    }
    else if (time_ref - traject->t3 <= 0) {
        // If we are here, then we are in the deceleration phase
        *rate_ref = traject->w1 + timest(traject->a2,    time_ref-traject->t2);
        *count_ref = traject->th2  + timest(traject->w1, time_ref-traject->t2) + timest2(traject->a2, time_ref-traject->t2);
    }
    else {
        // If we are here, we are in the zero speed phase (relevant when holding position)
        *rate_ref = 0;
        *count_ref = traject->th3;
    }
}
