#include <stdio.h>
#include <stdbool.h>

/* SAFETY CONDITION
 * 	(brake_pressure >= 0 && brake_pressure <= MAX_BRAKE_PRESSURE)
 * &&
 * 	(brake_pressure <= driver_brake_request)
 * &&
 * 	(mode != ABS_FAULT || brake_pressure == driver_brake_request)
 * &&
 * 	!(slip > slip_target && brake_pressure > prev_pressure)
 * &&
 * 	!(mode == ABS_RECOVERY && brake_pressure > prev_pressure + PRESSURE_STEP_UP);
 */

#define MAX_BRAKE_PRESSURE 255

#define NORMAL_SLIP_TARGET 15
#define ICE_SLIP_TARGET 5

#define PRESSURE_STEP_UP 20
#define PRESSURE_STEP_DOWN 40

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    ABS_NORMAL,
    ABS_FAULT,
    ABS_RECOVERY
} abs_mode_t;

/* Simulated hardware inputs */
volatile int wheel_speed_kph;
volatile int vehicle_speed_kph;
volatile int driver_brake_request;
volatile bool ice_mode_enabled;   // CRV candidate
volatile bool abs_system_fault;

/* Compute slip percentage */
int compute_slip(void) {
    if (vehicle_speed_kph <= 0) return 0;
    return 100 * (vehicle_speed_kph - wheel_speed_kph) / vehicle_speed_kph;
}

/* One control step */
int step_control_logic(
    int prev_pressure,
    abs_mode_t *mode
) {
    int new_pressure = prev_pressure;

    /* --- Mode transitions --- */
    if (abs_system_fault) {
        *mode = ABS_FAULT;
    }
    else if (*mode == ABS_FAULT && !abs_system_fault) {
        *mode = ABS_RECOVERY;
    }

    /* --- Control logic per mode --- */
    if (*mode == ABS_FAULT) {
        /* Hard override: CRVs ignored */
        new_pressure = driver_brake_request;
    }
    else {
        int slip = compute_slip();
        int slip_target = ice_mode_enabled ? ICE_SLIP_TARGET : NORMAL_SLIP_TARGET;

        if (driver_brake_request == 0) {
            new_pressure = 0;
        }
        else if (slip > slip_target) {
            new_pressure -= PRESSURE_STEP_DOWN;
        }
        else {
            new_pressure += PRESSURE_STEP_UP;
        }
    }

    /* --- Saturation & physical limits --- */
    if (new_pressure > driver_brake_request)
        new_pressure = driver_brake_request;
    if (new_pressure > MAX_BRAKE_PRESSURE)
        new_pressure = MAX_BRAKE_PRESSURE;
    if (new_pressure < 0)
        new_pressure = 0;

    return new_pressure;
}

int main() {
    int brake_pressure = 0;
    int prev_pressure = 0;
    abs_mode_t mode = ABS_NORMAL;

    printf("--- Refined ABS Controller ---\n");

    for (int iter = 0; iter < 200; iter++) {

        /* Deterministic safety scenario */
        vehicle_speed_kph = 100;
        wheel_speed_kph = 90 - (iter % 10);  // induces slip cycles
        driver_brake_request = 180;
        ice_mode_enabled = (iter > 60);
        abs_system_fault = (iter < 15);      // startup fault

        prev_pressure = brake_pressure;
        brake_pressure = step_control_logic(prev_pressure, &mode);

        int slip = compute_slip();
        int slip_target = ice_mode_enabled ? ICE_SLIP_TARGET : NORMAL_SLIP_TARGET;

        printf("iter=%d mode=%d slip=%d pressure=%d\n", iter, mode, slip, brake_pressure);
    }

    return 0;
}

