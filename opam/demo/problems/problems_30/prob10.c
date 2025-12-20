#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 * 	(gate_opening >= MIN_GATE_OPENING && gate_opening <= MAX_GATE_OPENING)
 * &&
 * 	(!(upstream_flood_detected) || (mode == EMERGENCY_OPEN && gate_opening == EMERGENCY_OPENING))
 * &&
 * 	(!(mode == POST_FLOOD_RECOVERY) || (gate_opening <= prev_gate_opening))
 */

#define MAX_GATE_OPENING 100
#define MIN_GATE_OPENING 0
#define EMERGENCY_OPENING 100

typedef enum {
    NORMAL,
    POST_FLOOD_RECOVERY,
    EMERGENCY_OPEN
} gate_mode_t;

/* Simulated hardware inputs */
volatile bool upstream_flood_detected;
volatile int reservoir_level;       // 0-100%
volatile int seasonal_water_demand; // 0-100 (CRV candidate)

void read_dam_sensors() {
    upstream_flood_detected = (rand() % 10 == 0); // 10% chance flood
    reservoir_level = 70 + (rand() % 35);
    seasonal_water_demand = rand() % 101;
}

void log_gate_state(const char* reason, int opening) {
    printf("Reason: %-25s | Gate Opening: %d%%\n", reason, opening);
}

int step_control_logic(int prev_gate_opening, gate_mode_t *mode) {
    int new_gate_opening = prev_gate_opening;

    /* --- Mode transitions --- */
    if (upstream_flood_detected) {
        *mode = EMERGENCY_OPEN;
    } else if (*mode == EMERGENCY_OPEN && !upstream_flood_detected) {
        *mode = POST_FLOOD_RECOVERY;
    } else if (*mode == POST_FLOOD_RECOVERY) {
        *mode = NORMAL;
    }

    /* --- Control logic per mode --- */
    if (*mode == EMERGENCY_OPEN) {
        new_gate_opening = EMERGENCY_OPENING;
        log_gate_state("UPSTREAM FLOOD DETECTED", new_gate_opening);
    } 
    else if (*mode == POST_FLOOD_RECOVERY) {
        // Gradually lower gate from full opening to normal operation
        int target_opening = (reservoir_level > 95) ? 75 + (seasonal_water_demand / 4) : seasonal_water_demand;
        new_gate_opening = prev_gate_opening - 5;
        if (new_gate_opening < target_opening) new_gate_opening = target_opening;
        log_gate_state("POST-FLOOD RECOVERY", new_gate_opening);
    } 
    else { /* NORMAL */
        if (reservoir_level > 95) {
            new_gate_opening = 75 + (seasonal_water_demand / 4);
            log_gate_state("High Reservoir Level", new_gate_opening);
        } else {
            new_gate_opening = seasonal_water_demand;
            log_gate_state("Seasonal Demand", new_gate_opening);
        }
    }

    /* --- Final Safety Saturation --- */
    if (new_gate_opening > MAX_GATE_OPENING) new_gate_opening = MAX_GATE_OPENING;
    if (new_gate_opening < MIN_GATE_OPENING) new_gate_opening = MIN_GATE_OPENING;

    return new_gate_opening;
}

int main() {
    srand(time(NULL));
    int gate_opening = 0;
    gate_mode_t mode = NORMAL;
    printf("--- Refined Dam Spillway Gate Control Simulation ---\n");

    for (int iter = 0; iter < 20; ++iter) {
        read_dam_sensors();
        int prev_gate_opening = gate_opening;
        gate_opening = step_control_logic(prev_gate_opening, &mode);

        printf("iter=%d flood=%d mode=%d gate=%d\n", iter, upstream_flood_detected, mode, gate_opening);
    }
    return 0;
}

