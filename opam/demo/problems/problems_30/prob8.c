#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION: 
        (0<=heater_duty_cycle &&  heater_duty_cycle<HEATER_MAX_DUTY_CYCLE)
 */

// --- Constants and Definitions ---
#define HEATER_MAX_DUTY_CYCLE 100
#define COOLING_MAX_PERCENT 100

#define PRESSURE_CRITICAL_PSI 500
#define PRESSURE_WARNING_PSI 460

#define PHASE_1_TEMP_TARGET 80.0f
#define PHASE_2_TEMP_TARGET 120.0f

#define MAX_TEMP_RATE_C_PER_STEP 8.0f   // Thermal runaway threshold
#define HEATER_RAMP_LIMIT 10            // Max duty change per step

// --- Simulated Hardware Inputs ---
volatile float current_temp_c;
volatile float last_temp_c;
volatile float current_pressure_psi;
volatile int reaction_phase;   // 1 or 2 (CRV candidate)
volatile bool system_enabled;

// --- Logging Helper ---
void log_reactor_state(const char* reason, int heater, int cooler) {
    printf(
        "Logic: %-28s | Heater: %3d%% | Cooler: %3d%% | T=%.1fC | P=%.0f PSI\n",
        reason, heater, cooler, current_temp_c, current_pressure_psi
    );
}

/**
 * @brief Main control logic for heater duty cycle.
 */
int step_heater(int last_heater_duty) {
    int new_heater_duty = last_heater_duty;
    int cooler_output = 0;
    float target_temp = 0.0f;

    float temp_rate = current_temp_c - last_temp_c;

    // ------------------------------------------------------------------
    // 1. COMPOUND CRITICAL SAFETY OVERRIDE
    // Triggered by either:
    //  - Hard over-pressure
    //  - Pressure warning + rapid temperature rise (thermal runaway)
    //
    // This path makes reaction_phase and temperature targets irrelevant.
    // ------------------------------------------------------------------
    if (
        current_pressure_psi >= PRESSURE_CRITICAL_PSI ||
        (current_pressure_psi >= PRESSURE_WARNING_PSI &&
         temp_rate >= MAX_TEMP_RATE_C_PER_STEP)
    ) {
        new_heater_duty = 0;
        cooler_output = COOLING_MAX_PERCENT;
        log_reactor_state("CRITICAL SAFETY OVERRIDE", new_heater_duty, cooler_output);
        return new_heater_duty;
    }

    // ------------------------------------------------------------------
    // 2. NORMAL OPERATION (SYSTEM ENABLED)
    // ------------------------------------------------------------------
    if (system_enabled) {

        // CRV-dependent target selection
        if (reaction_phase == 1) {
            target_temp = PHASE_1_TEMP_TARGET;
        } else {
            target_temp = PHASE_2_TEMP_TARGET;
        }

        float temp_error = target_temp - current_temp_c;

        // Proportional control with asymmetric response
        if (temp_error > 2.0f) {
            new_heater_duty += (int)(temp_error * 1.5f);
        } 
        else if (temp_error < -2.0f) {
            new_heater_duty -= (int)(-temp_error * 2.0f);
            cooler_output = (int)(-temp_error * 4.0f);
        }

        // Soft pressure derating
        if (current_pressure_psi > PRESSURE_WARNING_PSI) {
            new_heater_duty /= 2;
            log_reactor_state("PRESSURE DERATING", new_heater_duty, cooler_output);
        } else {
            log_reactor_state("NORMAL CONTROL", new_heater_duty, cooler_output);
        }
    }
    // ------------------------------------------------------------------
    // 3. SYSTEM DISABLED
    // ------------------------------------------------------------------
    else {
        new_heater_duty = 0;
        cooler_output = 0;
        log_reactor_state("SYSTEM DISABLED", new_heater_duty, cooler_output);
    }

    // ------------------------------------------------------------------
    // 4. RATE LIMITING (Actuator Protection)
    // ------------------------------------------------------------------
    if (new_heater_duty - last_heater_duty > HEATER_RAMP_LIMIT) {
        new_heater_duty = last_heater_duty + HEATER_RAMP_LIMIT;
    } else if (last_heater_duty - new_heater_duty > HEATER_RAMP_LIMIT) {
        new_heater_duty = last_heater_duty - HEATER_RAMP_LIMIT;
    }

    // ------------------------------------------------------------------
    // 5. FINAL SAFETY SATURATION
    // ------------------------------------------------------------------
    if (new_heater_duty > HEATER_MAX_DUTY_CYCLE)
        new_heater_duty = HEATER_MAX_DUTY_CYCLE;
    if (new_heater_duty < 0)
        new_heater_duty = 0;

    return new_heater_duty;
}

int main() {
    int heater_duty_cycle = 0;
    last_temp_c = 70.0f;

    printf("--- Refined Chemical Reactor Control Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        last_temp_c = current_temp_c;

        current_temp_c = 70.0f + (rand() % 60);    
        current_pressure_psi = 420 + (rand() % 150);
        reaction_phase = 1 + (rand() % 2);
        system_enabled = rand() % 2;

        heater_duty_cycle = step_heater(heater_duty_cycle);
    }

    return 0;
}

