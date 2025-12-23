#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 * 	(pump_rate > 0) 
 * && 
 * 	(air_in_line_detected || high_rate_counter > 0);
 */

// --- Constants ---
#define MAX_PUMP_RATE_ML_HR 500
#define DRUG_PROFILE_A_MAX_RATE 100
#define DRUG_PROFILE_B_MAX_RATE 250
#define HIGH_RATE_THRESHOLD_PERCENT 0.9
#define HIGH_RATE_CYCLES_LIMIT 5

// --- Simulated Hardware Inputs ---
volatile int target_rate_ml_hr;      // Desired rate from user input
volatile bool air_in_line_detected;  // Critical safety sensor
volatile int drug_profile_id;        // 1 for A, 2 for B (CRV candidate)
volatile bool pump_enabled;          // Master switch

// --- Function Prototypes ---
void read_pump_sensors();
void log_pump_state(const char* reason, int rate);
int step(int last_pump_rate, int* high_rate_counter);

int main() {
    srand(time(NULL));
    int pump_rate = 0;
    int high_rate_counter = 0;

    printf("--- Refined Medical Infusion Pump Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        // Simulate sensors
        target_rate_ml_hr = rand() % (MAX_PUMP_RATE_ML_HR + 1);
        air_in_line_detected = rand() % 2;   // 0 or 1
        drug_profile_id = (rand() % 2) + 1;  // 1 or 2
        pump_enabled = rand() % 2;           // 0 or 1

        // Compute new pump rate
        pump_rate = step(pump_rate, &high_rate_counter);

        // Evaluate safety property
        bool phi = (pump_rate > 0) && (air_in_line_detected || high_rate_counter > 0);
    }

    return 0;
}

// --- Read simulated hardware sensors (stub for real sensors) ---
void read_pump_sensors() {
    target_rate_ml_hr = rand() % (MAX_PUMP_RATE_ML_HR + 1);
    air_in_line_detected = rand() % 2;
    drug_profile_id = (rand() % 2) + 1;
    pump_enabled = rand() % 2;
}

// --- Log the pump state for debugging ---
void log_pump_state(const char* reason, int rate) {
    printf("Logic: %-30s | Pump Rate: %3d ml/hr\n", reason, rate);
}

// --- Step function with complex logic and safety ---
int step(int last_pump_rate, int* high_rate_counter) {
    int new_rate = 0;

    // --- CRITICAL SAFETY OVERRIDE: Air in line detected ---
    if (air_in_line_detected) {
        new_rate = 0;
        *high_rate_counter = 0;
        log_pump_state("AIR-IN-LINE ALARM", new_rate);
    }
    // --- Standard operational logic ---
    else if (pump_enabled) {
        int profile_max_rate = (drug_profile_id == 1) ? DRUG_PROFILE_A_MAX_RATE : DRUG_PROFILE_B_MAX_RATE;

        // Gradual ramp-up: limit rate increase per step
        if (target_rate_ml_hr > profile_max_rate) {
            new_rate = profile_max_rate;
        } else if (target_rate_ml_hr > last_pump_rate + 10) {
            new_rate = last_pump_rate + 10;
        } else {
            new_rate = target_rate_ml_hr;
        }

        // Non-trivial safety: sustained high-rate lockout
        if (new_rate >= profile_max_rate * HIGH_RATE_THRESHOLD_PERCENT) {
            (*high_rate_counter)++;
            if (*high_rate_counter > HIGH_RATE_CYCLES_LIMIT) {
                new_rate = 0;
                log_pump_state("SUSTAINED HIGH RATE LOCKOUT", new_rate);
                *high_rate_counter = 0;
            } else {
                log_pump_state("High rate, monitoring", new_rate);
            }
        } else {
            *high_rate_counter = 0;
            log_pump_state("Normal operation", new_rate);
        }
    }
    // --- Pump disabled ---
    else {
        new_rate = 0;
        *high_rate_counter = 0;
        log_pump_state("Pump disabled", new_rate);
    }

    // --- Final safety saturation ---
    if (new_rate < 0) new_rate = 0;
    if (new_rate > MAX_PUMP_RATE_ML_HR) new_rate = MAX_PUMP_RATE_ML_HR;

    return new_rate;
}

