#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION
 *	(compressor_state == COMPRESSOR_ON || compressor_state == COMPRESSOR_OFF)
 * &&
 * 	(!water_leak_detected || compressor_state == COMPRESSOR_OFF)
 * &&
 * 	(mode != LEAK_SHUTDOWN || compressor_state == COMPRESSOR_OFF)
 * &&
 * 	(mode != COOLDOWN_RECOVERY || compressor_state == COMPRESSOR_OFF)
 * &&
 * 	!(prev_state == COMPRESSOR_OFF && compressor_state == COMPRESSOR_ON && mode != NORMAL_OPERATION)
 * &&
 * 	(!unit_enabled_by_master || compressor_state == COMPRESSOR_OFF || mode == NORMAL_OPERATION);
 */

#define COMPRESSOR_ON  1
#define COMPRESSOR_OFF 0

#define TEMP_HIGH_THRESHOLD_C 40.0f
#define TEMP_LOW_THRESHOLD_C  35.0f
#define COOLDOWN_CYCLES 5

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL_OPERATION,
    LEAK_SHUTDOWN,
    COOLDOWN_RECOVERY
} cooling_mode_t;

/* Simulated hardware inputs */
volatile float rack_inlet_temp_c;
volatile bool water_leak_detected;
volatile int server_cpu_load_percent;   // CRV candidate
volatile bool unit_enabled_by_master;

/* Control state */
volatile int cooldown_timer = 0;

int step_control_logic(
    int prev_state,
    cooling_mode_t *mode
) {
    int new_state = prev_state;

    /* --- Mode transitions --- */
    if (water_leak_detected) {
        *mode = LEAK_SHUTDOWN;
        cooldown_timer = COOLDOWN_CYCLES;
    }
    else if (*mode == LEAK_SHUTDOWN && !water_leak_detected) {
        *mode = COOLDOWN_RECOVERY;
    }
    else if (*mode == COOLDOWN_RECOVERY && cooldown_timer == 0) {
        *mode = NORMAL_OPERATION;
    }

    /* --- Control logic per mode --- */
    if (*mode == LEAK_SHUTDOWN) {
        new_state = COMPRESSOR_OFF;
    }
    else if (*mode == COOLDOWN_RECOVERY) {
        new_state = COMPRESSOR_OFF;
        if (cooldown_timer > 0)
            cooldown_timer--;
    }
    else { /* NORMAL_OPERATION */
        if (!unit_enabled_by_master) {
            new_state = COMPRESSOR_OFF;
        }
        else if (server_cpu_load_percent > 90 ||
                 rack_inlet_temp_c > TEMP_HIGH_THRESHOLD_C) {
            new_state = COMPRESSOR_ON;
        }
        else if (rack_inlet_temp_c < TEMP_LOW_THRESHOLD_C) {
            new_state = COMPRESSOR_OFF;
        }
        else {
            new_state = prev_state; /* hysteresis */
        }
    }

    return new_state;
}

int main() {
    int compressor_state = COMPRESSOR_OFF;
    int prev_state = COMPRESSOR_OFF;
    cooling_mode_t mode = NORMAL_OPERATION;

    printf("--- Refined Data Center Cooling Controller ---\n");

    for (int iter = 0; iter < 200; iter++) {

        /* Deterministic safety scenario */
        if (iter >= 40 && iter < 60)
            water_leak_detected = true;
        else
            water_leak_detected = false;

        rack_inlet_temp_c = 34.0f + (rand() % 10);  // 34–43 C
        server_cpu_load_percent = 50 + (rand() % 51);
        unit_enabled_by_master = (rand() % 2);

        prev_state = compressor_state;
        compressor_state = step_control_logic(prev_state, &mode);

        printf("iter=%d leak=%d mode=%d temp=%.1f load=%d cmd=%d cooldown=%d\n",iter,water_leak_detected,mode,rack_inlet_temp_c,server_cpu_load_percent,compressor_state,cooldown_timer);
    }
    return 0;
}

