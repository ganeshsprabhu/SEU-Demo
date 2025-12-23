#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION:
 * 	(fuel_injection_ms >= 0)
 * &&
 * 	(!(engine_temp_c >= TEMP_CRITICAL_C)
 * &&
 * 	(engine_rpm <= TEMP_OVERRIDE_RPM_LIMIT || fuel_injection_ms == 0))
 * &&
 * 	(!(engine_rpm > MAX_RPM_HARD_LIMIT) || fuel_injection_ms == 0);
 */

// --- Constants and Definitions ---
#define MAX_RPM_HARD_LIMIT 9000
#define RPM_LIMIT_ECO 6000
#define RPM_LIMIT_SPORT 8500
#define TEMP_CRITICAL_C 120
#define TEMP_WARNING_C 105
#define TEMP_OVERRIDE_RPM_LIMIT 3000
#define MAX_FUEL_INJECTION_MS 20

// --- Simulated Hardware Inputs ---
volatile int engine_rpm;
volatile int engine_temp_c;
volatile int driver_mode;         // 0: Eco, 1: Sport (CRV candidate)
volatile int throttle_position;   // 0–100%

// --- Function Prototypes ---
void log_ecu_state(const char* reason, int rpm_limit, int fuel_ms);
int step(int last_fuel_injection_ms);

/**
 * @brief Logs ECU decisions.
 */
void log_ecu_state(const char* reason, int rpm_limit, int fuel_ms) {
    printf("Logic: %-25s | RPM Limit: %4d | Fuel: %2d ms\n",
           reason, rpm_limit, fuel_ms);
}

/**
 * @brief Main ECU control step.
 */
int step(int last_fuel_injection_ms) {
    int active_rpm_limit;
    int new_fuel_injection = last_fuel_injection_ms;

    // 1. CRITICAL SAFETY OVERRIDE — Engine Over-temperature
    // CRV 'driver_mode' becomes irrelevant here.
    if (engine_temp_c >= TEMP_CRITICAL_C) {
        active_rpm_limit = TEMP_OVERRIDE_RPM_LIMIT;
        if (engine_rpm > active_rpm_limit) {
            new_fuel_injection = 0; // Hard fuel cut
        } else {
            new_fuel_injection = 2; // Limp-home fueling
        }
        log_ecu_state("CRITICAL OVERHEAT", active_rpm_limit, new_fuel_injection);
    }

    // 2. THERMAL DERATING REGION (complex intermediate logic)
    else if (engine_temp_c >= TEMP_WARNING_C) {
        // Gradually derate RPM limit based on temperature slope
        int derate = (engine_temp_c - TEMP_WARNING_C) * 300;
        active_rpm_limit = RPM_LIMIT_ECO - derate;

        if (engine_rpm > active_rpm_limit) {
            new_fuel_injection = 0;
        } else {
            new_fuel_injection = throttle_position / 15;
        }
        log_ecu_state("THERMAL DERATING", active_rpm_limit, new_fuel_injection);
    }

    // 3. NORMAL OPERATION
    else {
        // CRV-dependent logic
        if (driver_mode == 0) {
            active_rpm_limit = RPM_LIMIT_ECO;
        } else {
            active_rpm_limit = RPM_LIMIT_SPORT;
        }

        // RPM limiting logic
        if (engine_rpm > active_rpm_limit || engine_rpm > MAX_RPM_HARD_LIMIT) {
            new_fuel_injection = 0;
        } else {
            new_fuel_injection = throttle_position / 10;
        }
        log_ecu_state("NORMAL OPERATION", active_rpm_limit, new_fuel_injection);
    }

    // 4. FINAL SATURATION
    if (new_fuel_injection < 0) new_fuel_injection = 0;
    if (new_fuel_injection > MAX_FUEL_INJECTION_MS)
        new_fuel_injection = MAX_FUEL_INJECTION_MS;

    return new_fuel_injection;
}

/**
 * @brief Main execution loop.
 */
int main() {
    int fuel_injection_ms = 0;
    printf("--- Advanced ECU RPM Limiter Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        engine_rpm = 4000 + rand() % 6000;       // 4000–9999 RPM
        engine_temp_c = 90 + rand() % 55;        // 90–144 °C
        driver_mode = rand() % 2;                // Eco / Sport
        throttle_position = rand() % 101;        // 0–100%

        fuel_injection_ms = step(fuel_injection_ms);


        printf("\n");
    }

    return 0;
}

