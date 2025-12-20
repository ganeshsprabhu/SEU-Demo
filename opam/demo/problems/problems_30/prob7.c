#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION:
 * 	(charge_current >= 0 && charge_current <= MAX_CHARGE_CURRENT_MA)
 * &&
 * 	((max_cell_voltage_mv <= CELL_OVERVOLTAGE_MV && pack_temp_c <= MAX_SAFE_TEMP_C) || charge_current == 0)
 * &&
 * 	(mode != BMS_FAULT || charge_current == 0)
 * &&
 * 	(mode != BMS_COOLDOWN || prev_current == 0)
 * &&
 * 	!(prev_current > 0 && charge_current > 0 && (max_cell_voltage_mv > CELL_OVERVOLTAGE_MV || pack_temp_c > MAX_SAFE_TEMP_C));
 */

#define MAX_CHARGE_CURRENT_MA 5000
#define CELL_OVERVOLTAGE_MV 4250
#define MAX_SAFE_TEMP_C 55.0f

#define PROFILE_LONGEVITY_MA 1500
#define PROFILE_FAST_MA 4000

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    BMS_NORMAL,
    BMS_COOLDOWN,
    BMS_FAULT
} bms_mode_t;

/* --- Simulated Hardware Inputs --- */
volatile int max_cell_voltage_mv;
volatile float pack_temp_c;
volatile int charge_profile;     // 0: longevity, 1: fast (CRV candidate)
volatile bool charger_connected;

/* --- Control Step --- */
int step_bms_logic(
    int prev_current,
    bms_mode_t *mode
) {
    int new_current = prev_current;

    /* --- Mode transitions --- */
    if (max_cell_voltage_mv > CELL_OVERVOLTAGE_MV ||
        pack_temp_c > MAX_SAFE_TEMP_C) {
        *mode = BMS_FAULT;
    }
    else if (*mode == BMS_FAULT) {
        *mode = BMS_COOLDOWN;
    }

    /* --- Control logic per mode --- */
    if (*mode == BMS_FAULT) {
        /* Hard safety override */
        new_current = 0;
    }
    else if (*mode == BMS_COOLDOWN) {
        /* Require one full zero-current cycle */
        if (prev_current == 0 && charger_connected) {
            *mode = BMS_NORMAL;
            new_current = 0;
        } else {
            new_current = 0;
        }
    }
    else { /* BMS_NORMAL */
        if (!charger_connected) {
            new_current = 0;
        } else {
            int target_current =
                (charge_profile == 0)
                    ? PROFILE_LONGEVITY_MA
                    : PROFILE_FAST_MA;

            /* Temperature-based tapering */
            if (pack_temp_c > 45.0f) {
                target_current /= 2;
            }

            new_current = target_current;
        }
    }

    /* --- Saturation --- */
    if (new_current > MAX_CHARGE_CURRENT_MA)
        new_current = MAX_CHARGE_CURRENT_MA;
    if (new_current < 0)
        new_current = 0;

    return new_current;
}

int main() {
    int charge_current = 0;
    int prev_current = 0;
    bms_mode_t mode = BMS_NORMAL;

    printf("--- Refined BMS Controller Simulation ---\n");

    for (int iter = 0; iter < 200; iter++) {

        /* Deterministic fault injection */
        if (iter >= 50 && iter < 80) {
            max_cell_voltage_mv = 4300;  // force over-voltage fault
            pack_temp_c = 40.0f;
        } else {
            max_cell_voltage_mv = 4000 + rand() % 200;
            pack_temp_c = 25.0f + (rand() % 300) / 10.0f;
        }

        charge_profile = rand() % 2;
        charger_connected = (rand() % 2) == 1;

        prev_current = charge_current;
        charge_current = step_bms_logic(prev_current, &mode);

        printf("iter=%d Vcell=%d temp=%.1f profile=%d mode=%d current=%d\n",iter, max_cell_voltage_mv, pack_temp_c,charge_profile, mode, charge_current);
    }

    return 0;
}

