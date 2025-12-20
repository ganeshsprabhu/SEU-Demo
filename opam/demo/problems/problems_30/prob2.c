#include <stdio.h>
#include <stdbool.h>
/* SAFETY CONDITION: 
 * 	(anesthetic_percentage >= MIN_ANESTHETIC_PERCENT && anesthetic_percentage <= MAX_ANESTHETIC_PERCENT)
 * &&
 * 	(o2_supply_pressure >= LOW_O2_PRESSURE || anesthetic_percentage == 0)
 * &&
 * 	(mode != EMERGENCY_O2 || anesthetic_percentage == 0)
 * &&
 * 	(mode != RAMP_DOWN || anesthetic_percentage >= anesthetic_prev)
 * &&
 * 	((anesthetic_percentage - anesthetic_prev <= 2) && (anesthetic_prev - anesthetic_percentage <= 2));
 * /

#define MAX_ANESTHETIC_PERCENT 8
#define MIN_ANESTHETIC_PERCENT 0

#define LOW_O2_PRESSURE 30
#define RECOVERY_O2_PRESSURE 35

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL,
    RAMP_DOWN,
    EMERGENCY_O2
} machine_mode_t;

/* Simulated hardware / environment inputs */
volatile int o2_supply_pressure;
volatile int patient_age;
volatile int target_anesthetic_percent;

int step_control_logic(int prev_anesthetic_percent,machine_mode_t *mode)
{
    int new_anesthetic = prev_anesthetic_percent;

    /* --- Mode transitions --- */
    if (o2_supply_pressure < LOW_O2_PRESSURE) {
        *mode = EMERGENCY_O2;
    }
    else if (*mode == EMERGENCY_O2 &&
             o2_supply_pressure >= RECOVERY_O2_PRESSURE) {
        *mode = RAMP_DOWN;
    }

    /* --- Control logic per mode --- */
    if (*mode == EMERGENCY_O2) {
        /* Hard safety override */
        new_anesthetic = MIN_ANESTHETIC_PERCENT;
    }
    else if (*mode == RAMP_DOWN) {
        /* Gradual reintroduction */
        int ramp_rate = (patient_age > 65) ? 1 : 2;
        new_anesthetic += ramp_rate;

        if (new_anesthetic >= target_anesthetic_percent) {
            new_anesthetic = target_anesthetic_percent;
            *mode = NORMAL;
        }
    }
    else { /* NORMAL */
        /* Gentle tracking of patient setting */
        if (new_anesthetic < target_anesthetic_percent)
            new_anesthetic++;
        else if (new_anesthetic > target_anesthetic_percent)
            new_anesthetic--;
    }

    /* --- Final saturation --- */
    if (new_anesthetic > MAX_ANESTHETIC_PERCENT)
        new_anesthetic = MAX_ANESTHETIC_PERCENT;
    if (new_anesthetic < MIN_ANESTHETIC_PERCENT)
        new_anesthetic = MIN_ANESTHETIC_PERCENT;

    return new_anesthetic;
}

int main() {
    int anesthetic_percentage = 0;
    int anesthetic_prev = 0;
    machine_mode_t mode = NORMAL;

    /* Fixed patient data (CRV-like input) */
    patient_age = 72;
    target_anesthetic_percent = (patient_age > 60) ? 2 : 3;

    printf("--- Refined Anesthesia Gas Mixer Simulation ---\n");

    for (int iter = 0; iter < 20; iter++) {

        /* Deterministic pressure profile to exercise modes */
        if (iter < 5)
            o2_supply_pressure = 28;   // force emergency
        else if (iter < 10)
            o2_supply_pressure = 32;   // still unsafe (hysteresis)
        else
            o2_supply_pressure = 40;   // recovery

        anesthetic_prev = anesthetic_percentage;

        anesthetic_percentage =step_control_logic(anesthetic_prev, &mode);
	printf("anesthetic_percentage: %d\n", anesthetic_percentage);
    }
    return 0;
}

