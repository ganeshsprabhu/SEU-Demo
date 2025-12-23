#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION
    (gate_opening >= MIN_GATE_OPENING &&
     gate_opening <= MAX_GATE_OPENING)
  &&
    (!upstream_flood_detected ||
	gate_opening == EMERGENCY_OPENING)

  &&
    (mode != FLOOD_EMERGENCY ||
	gate_opening == EMERGENCY_OPENING)

  &&
    (abs(gate_opening - gate_prev) <= MAX_GATE_STEP ||
	gate_opening == EMERGENCY_OPENING)

  &&
    !(upstream_flood_detected &&
	 gate_opening < gate_prev);
*/

#define MAX_GATE_OPENING 100
#define MIN_GATE_OPENING 0
#define EMERGENCY_OPENING 100
#define MAX_GATE_STEP 10   // Max change per cycle

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL_OPERATION = 0,
    FLOOD_EMERGENCY  = 1,
    RECOVERY_MODE   = 2
} dam_mode_t;

/* Simulated hardware inputs */
volatile bool upstream_flood_detected;
volatile int reservoir_level;         // percentage
volatile int seasonal_water_demand;   // CRV candidate

void read_dam_sensors(int iter) {
    /* Deterministic scenario for safety reasoning */
    if (iter >= 20 && iter < 40)
        upstream_flood_detected = true;
    else
        upstream_flood_detected = false;

    if (iter < 60)
        reservoir_level = 90 + (iter % 15);
    else
        reservoir_level = 80;

    seasonal_water_demand = (iter * 7) % 101;
}

int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int step_control_logic(int prev_gate_opening,dam_mode_t *mode)
{
    int new_gate = prev_gate_opening;

    /* --- Mode transitions --- */
    if (upstream_flood_detected) {
        *mode = FLOOD_EMERGENCY;
    }
    else if (*mode == FLOOD_EMERGENCY) {
        *mode = RECOVERY_MODE;
    }

    /* --- Control logic per mode --- */
    if (*mode == FLOOD_EMERGENCY) {
        new_gate = EMERGENCY_OPENING;
    }
    else if (*mode == RECOVERY_MODE) {
        /* Gradually close gate after flood */
        if (prev_gate_opening > seasonal_water_demand) {
            new_gate = prev_gate_opening - MAX_GATE_STEP;
        } else {
            *mode = NORMAL_OPERATION;
        }
    }
    else { /* NORMAL_OPERATION */
        if (reservoir_level > 95) {
            new_gate = seasonal_water_demand + 20;
        } else {
            new_gate = seasonal_water_demand;
        }
    }

    return clamp(new_gate, MIN_GATE_OPENING, MAX_GATE_OPENING);
}

int main() {
    srand(time(NULL));

    int gate_opening = 0;
    int gate_prev = 0;
    dam_mode_t mode = NORMAL_OPERATION;

    printf("--- Refined Dam Spillway Gate Controller ---\n");

    for (int iter = 0; iter < 100; iter++) {
        read_dam_sensors(iter);

        gate_prev = gate_opening;
        gate_opening = step_control_logic(gate_prev, &mode);

        printf("iter=%d flood=%d mode=%d level=%d demand=%d gate=%d\n",iter,upstream_flood_detected,mode,reservoir_level,seasonal_water_demand,gate_opening);
    }
    return 0;
}

