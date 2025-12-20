#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION:
 * 	(motor_speed >= MIN_SPEED && motor_speed <= MAX_SPEED)
 * &&
 * 	(!jam_detected || motor_speed == STOP_SPEED)
 * &&
 * 	!(mode == SLOW_START && motor_speed != STOP_SPEED && prev_speed != STOP_SPEED)
 * &&
 * 	(abs(motor_speed - prev_speed) <= ACCEL_STEP);
 */

#define MAX_SPEED 100
#define MIN_SPEED 0
#define STOP_SPEED 0
#define ACCEL_STEP 10  // Maximum allowed speed change per iteration

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL,
    SLOW_START,
    EMERGENCY_STOP
} conveyor_mode_t;

/* Simulated hardware inputs */
volatile bool jam_detected;
volatile int item_weight; // CRV candidate

int step_control_logic(int prev_speed, conveyor_mode_t *mode) {
    int target_speed;

    // --- Mode transitions ---
    if (jam_detected) {
        *mode = EMERGENCY_STOP;
    } else if (*mode == EMERGENCY_STOP && !jam_detected) {
        *mode = SLOW_START;
    }

    // --- Control logic per mode ---
    if (*mode == EMERGENCY_STOP) {
        target_speed = STOP_SPEED;
    } else if (*mode == SLOW_START) {
        // Require one full STOP cycle before resuming
        if (prev_speed == STOP_SPEED) {
            target_speed = (item_weight > 30) ? 50 : 90;
            *mode = NORMAL;
        } else {
            target_speed = STOP_SPEED;
        }
    } else { // NORMAL
        target_speed = (item_weight > 30) ? 50 : 90;
    }

    // --- Smooth acceleration/deceleration ---
    if (target_speed > prev_speed + ACCEL_STEP) {
        target_speed = prev_speed + ACCEL_STEP;
    } else if (target_speed < prev_speed - ACCEL_STEP) {
        target_speed = prev_speed - ACCEL_STEP;
    }

    // --- Final safety saturation ---
    if (target_speed > MAX_SPEED) target_speed = MAX_SPEED;
    if (target_speed < MIN_SPEED) target_speed = MIN_SPEED;

    return target_speed;
}

int main() {
    int motor_speed = 0;
    int prev_speed = 0;
    conveyor_mode_t mode = NORMAL;

    printf("--- Refined Conveyor Belt Controller ---\n");

    for (int iter = 0; iter < 50; iter++) {
        // Simulated sensor inputs
        if (iter >= 20 && iter < 30)
            jam_detected = true; // Force emergency stop
        else
            jam_detected = false;

        item_weight = 10 + rand() % 50; // 10-59 kg

        prev_speed = motor_speed;
        motor_speed = step_control_logic(prev_speed, &mode);

        printf("iter=%d jam=%d mode=%d weight=%d prev_speed=%d speed=%d\n",iter, jam_detected, mode, item_weight, prev_speed, motor_speed);
    }
    return 0;
}

