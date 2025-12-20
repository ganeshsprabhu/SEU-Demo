#include <stdio.h>
#include <stdbool.h>

/* SAFETY CONDITION:
 * 	(blade_state == BLADE_OFF || blade_state == BLADE_ON)
 * &&
 * 	(tilt_angle <= MAX_TILT_ANGLE || blade_state == BLADE_OFF)
 * &&
 * 	(mode != MOWER_TILT_SHUTDOWN || blade_state == BLADE_OFF)
 * &&
 * 	(mode != MOWER_RECOVERY || blade_prev == BLADE_OFF)
 * &&
 * 	!(tilt_angle > SAFE_TILT_ANGLE && blade_state == BLADE_ON)
 */

#define BLADE_OFF 0
#define BLADE_ON  1

#define MAX_TILT_ANGLE 30
#define SAFE_TILT_ANGLE 20

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    MOWER_NORMAL,
    MOWER_TILT_SHUTDOWN,
    MOWER_RECOVERY
} mower_mode_t;

/* Simulated hardware inputs */
volatile int tilt_angle;
volatile bool follow_gps_path;   // CRV candidate

/* One control step */
int step_control_logic(
    int prev_blade_state,
    mower_mode_t *mode
) {
    int new_blade_state = prev_blade_state;

    /* --- Mode transitions --- */
    if (tilt_angle > MAX_TILT_ANGLE) {
        *mode = MOWER_TILT_SHUTDOWN;
    }
    else if (*mode == MOWER_TILT_SHUTDOWN &&
             tilt_angle <= SAFE_TILT_ANGLE) {
        *mode = MOWER_RECOVERY;
    }

    /* --- Control logic per mode --- */
    if (*mode == MOWER_TILT_SHUTDOWN) {
        /* Hard safety override: CRV ignored */
        new_blade_state = BLADE_OFF;
    }
    else if (*mode == MOWER_RECOVERY) {
        /* Require one stable OFF cycle before restarting blades */
        if (prev_blade_state == BLADE_OFF) {
            if (follow_gps_path) {
                new_blade_state = BLADE_ON;
                *mode = MOWER_NORMAL;
            } else {
                new_blade_state = BLADE_OFF;
            }
        } else {
            new_blade_state = BLADE_OFF;
        }
    }
    else { /* MOWER_NORMAL */
        if (follow_gps_path) {
            new_blade_state = BLADE_ON;
        } else {
            new_blade_state = BLADE_OFF;
        }
    }

    return new_blade_state;
}

int main() {
    int blade_state = BLADE_OFF;
    int blade_prev = BLADE_OFF;
    mower_mode_t mode = MOWER_NORMAL;

    printf("--- Refined Autonomous Lawn Mower Controller ---\n");

    for (int iter = 0; iter < 150; iter++) {

        /* Deterministic safety scenario */
        if (iter >= 30 && iter < 50)
            tilt_angle = 35;        // force unsafe tilt
        else if (iter >= 50 && iter < 60)
            tilt_angle = 25;        // recovery band
        else
            tilt_angle = 15;        // safe operation

        follow_gps_path = (iter < 120);

        blade_prev = blade_state;
        blade_state = step_control_logic(blade_prev, &mode);

        printf("iter=%d tilt=%d mode=%d blade=%d gps=%d\n",iter, tilt_angle, mode, blade_state, follow_gps_path);
    }
    return 0;
}

