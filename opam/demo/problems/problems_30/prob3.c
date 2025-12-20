#include <stdio.h>
#include <stdbool.h>

/* SAFETY CONDITION:
 *	(vent_opening >= MIN_VENT_OPENING && vent_opening <= MAX_VENT_OPENING)
 * &&
 * 	(wind_speed <= HIGH_WIND_THRESHOLD || vent_opening == MIN_VENT_OPENING)
 * &&
 * 	(mode != WIND_LOCK || vent_opening == MIN_VENT_OPENING)
 * &&
 * 	(mode != RAMP_OPEN || vent_opening >= vent_prev)
 * &&
 * 	((vent_opening - vent_prev <= MAX_VENT_STEP) && (vent_prev - vent_opening <= MAX_VENT_STEP));
 */

#define MAX_VENT_OPENING 100
#define MIN_VENT_OPENING 0

#define HIGH_WIND_THRESHOLD 50     // km/h
#define WIND_RECOVERY_THRESHOLD 40 // hysteresis

#define MAX_VENT_STEP 20           // max % change per iteration

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL,
    RAMP_OPEN,
    WIND_LOCK
} vent_mode_t;

/* Simulated hardware inputs */
volatile int wind_speed;
volatile int current_temp;
volatile int target_temp; // CRV candidate

int step_control_logic(int prev_vent_opening,vent_mode_t *mode)
{
    int new_vent = prev_vent_opening;

    /* --- Mode transitions --- */
    if (wind_speed > HIGH_WIND_THRESHOLD) {
        *mode = WIND_LOCK;
    }
    else if (*mode == WIND_LOCK &&
             wind_speed <= WIND_RECOVERY_THRESHOLD) {
        *mode = RAMP_OPEN;
    }

    /* --- Control logic per mode --- */
    if (*mode == WIND_LOCK) {
        /* Hard safety override */
        new_vent = MIN_VENT_OPENING;
    }
    else if (*mode == RAMP_OPEN) {
        /* Gradual reopening */
        int temp_diff = current_temp - target_temp;
        int desired = (temp_diff > 0) ? temp_diff * 10 : 0;

        if (new_vent < desired)
            new_vent += MAX_VENT_STEP;
        else
            *mode = NORMAL;
    }
    else { /* NORMAL */
        int temp_diff = current_temp - target_temp;
        if (temp_diff > 0)
            new_vent = temp_diff * 10;
        else
            new_vent = prev_vent_opening;
    }

    /* --- Final saturation --- */
    if (new_vent > MAX_VENT_OPENING)
        new_vent = MAX_VENT_OPENING;
    if (new_vent < MIN_VENT_OPENING)
        new_vent = MIN_VENT_OPENING;

    return new_vent;
}

int main() {
    int vent_opening = 0;
    int vent_prev = 0;
    vent_mode_t mode = NORMAL;

    target_temp = 25;

    printf("--- Refined Greenhouse Vent Controller ---\n");

    for (int iter = 0; iter < 20; iter++) {

        /* Deterministic environmental profile */
        if (iter < 6)
            wind_speed = 60;   // force wind lock
        else if (iter < 10)
            wind_speed = 45;   // hysteresis zone
        else
            wind_speed = 30;   // safe

        current_temp = 20 + iter; // gradually warming greenhouse

        vent_prev = vent_opening;
        vent_opening = step_control_logic(vent_prev, &mode);


        printf("iter=%d wind=%d temp=%d mode=%d vent=%d\n",iter, wind_speed, current_temp, mode, vent_opening);
    }
    return 0;
}

