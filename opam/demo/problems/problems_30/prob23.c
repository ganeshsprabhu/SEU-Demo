// pipeline_pressure_regulator.c
// Advanced pipeline pressure regulation with safety invariants.
// Normal operation regulates valve position based on downstream flow demand (CRV).
// Safety overrides include overpressure protection, rate limiting, hysteresis,
// and a latched emergency shutdown to prevent oscillatory or unsafe behavior.
//
// SAFETY DOMAIN:
//   0 <= valve_position <= 100
//   Overpressure must force valve closed within one step
//   Valve opening rate is bounded
//   Valve must remain closed while in emergency shutdown
//
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 *	(valve_position >= MIN_VALVE_POSITION && valve_position <= MAX_VALVE_POSITION)
 * && 
 * 	(!(upstream_pressure > OVERPRESSURE_THRESHOLD) || valve_position == MIN_VALVE_POSITION)
 * && 
 * 	(!emergency_shutdown_latched || valve_position == MIN_VALVE_POSITION)
 * && 
 * 	(abs(valve_position - last_valve_position) <= MAX_CLOSE_RATE)
 */

// ---------------- Constants ----------------
#define MAX_VALVE_POSITION 100
#define MIN_VALVE_POSITION 0

#define OVERPRESSURE_THRESHOLD 800   // PSI
#define PRESSURE_HYSTERESIS   50    // PSI

#define MAX_OPEN_RATE   15          // % per step
#define MAX_CLOSE_RATE  25          // % per step

#define SENSOR_FAULT_THRESHOLD 1100 // PSI (implausible reading)

// ---------------- Simulated Inputs ----------------
volatile int upstream_pressure;
volatile int downstream_demand;     // CRV candidate (0–100)

// ---------------- Internal Controller State ----------------
bool emergency_shutdown_latched = false;
int last_valve_position = 50;

// ---------------- Sensor Simulation ----------------
void read_pipeline_sensors() {
    upstream_pressure = 720 + (rand() % 200);   // 720–920 PSI
    downstream_demand = rand() % 101;           // 0–100 %
}

// ---------------- Logging ----------------
void log_valve_state(const char* reason, int position) {
    printf("Logic: %-30s | Valve: %3d%% | Pressure: %4d PSI\n",
           reason, position, upstream_pressure);
}

// ---------------- Control Logic ----------------
int step_control_logic() {
    int desired_position;
    int new_position = last_valve_position;

    // 1. HARD SAFETY: Sensor plausibility check
    if (upstream_pressure > SENSOR_FAULT_THRESHOLD) {
        emergency_shutdown_latched = true;
        desired_position = MIN_VALVE_POSITION;
        log_valve_state("SENSOR FAULT - LOCKOUT", desired_position);
    }
    // 2. CRITICAL SAFETY OVERRIDE: Overpressure
    else if (upstream_pressure > OVERPRESSURE_THRESHOLD) {
        emergency_shutdown_latched = true;
        desired_position = MIN_VALVE_POSITION;
        log_valve_state("OVERPRESSURE SHUTDOWN", desired_position);
    }
    // 3. LATCHED EMERGENCY MODE
    else if (emergency_shutdown_latched) {
        // Only clear latch once pressure is well below threshold
        if (upstream_pressure < OVERPRESSURE_THRESHOLD - PRESSURE_HYSTERESIS) {
            emergency_shutdown_latched = false;
            desired_position = downstream_demand;
            log_valve_state("RECOVERY FROM SHUTDOWN", desired_position);
        } else {
            desired_position = MIN_VALVE_POSITION;
            log_valve_state("EMERGENCY LATCH ACTIVE", desired_position);
        }
    }
    // 4. NORMAL OPERATION (CRV-dependent)
    else {
        desired_position = downstream_demand;
        log_valve_state("MATCHING DEMAND", desired_position);
    }

    // 5. RATE LIMITING (prevents mechanical stress)
    if (desired_position > last_valve_position) {
        int delta = desired_position - last_valve_position;
        if (delta > MAX_OPEN_RATE)
            new_position = last_valve_position + MAX_OPEN_RATE;
        else
            new_position = desired_position;
    } else {
        int delta = last_valve_position - desired_position;
        if (delta > MAX_CLOSE_RATE)
            new_position = last_valve_position - MAX_CLOSE_RATE;
        else
            new_position = desired_position;
    }

    // 6. FINAL SATURATION
    if (new_position > MAX_VALVE_POSITION) new_position = MAX_VALVE_POSITION;
    if (new_position < MIN_VALVE_POSITION) new_position = MIN_VALVE_POSITION;

    last_valve_position = new_position;
    return new_position;
}

// ---------------- Main ----------------
int main() {
    srand(time(NULL));
    int valve_position = last_valve_position;

    printf("--- Advanced Pipeline Pressure Regulator Simulation ---\n");

    for (int i = 0; i < 30; i++) {
        read_pipeline_sensors();
        valve_position = step_control_logic();

    }

    return 0;
}

