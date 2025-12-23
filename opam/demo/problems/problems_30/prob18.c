#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

/* SAFETY CONDITION:
 *	(damper_position >= 0 && damper_position <= 100)
 * &&
 * 	(!fire_alarm_confirmed || damper_position == 0)
 * &&
 * 	(system_on || damper_position == 0)
 * &&
 *	(abs(damper_position - last_damper_position) <= MAX_STEP)
 */

// --- Constants and Definitions ---
#define DAMPER_MAX_OPEN 100
#define DAMPER_MIN_OPEN 0
#define DAMPER_FIRE_SAFE_POSITION 0

#define MAX_STEP 15                 // Max % movement per control cycle
#define TEMP_DEADBAND 1.0f          // ±1°F deadband
#define FIRE_PREALARM_COUNT 3       // Consecutive cycles before confirmed alarm

// --- Simulated Hardware Inputs ---
volatile float current_temp_f;
volatile float target_temp_f;
volatile bool fire_alarm_signal;     // Raw fire signal
volatile bool system_on;

// --- Internal Controller State ---
volatile int fire_alarm_counter = 0;
volatile bool fire_alarm_confirmed = false;

// --- Function Prototypes ---
void read_hvac_sensors();
void log_hvac_state(const char* reason, int position);
int clamp(int val, int min, int max);
int step(int last_damper_position);

/**
 * @brief Simulates reading HVAC and building safety sensors.
 */
void read_hvac_sensors() {
    current_temp_f = 65.0f + (rand() % 21); // 65–85 F
    target_temp_f  = 68.0f + (rand() % 7);  // 68–74 F
    fire_alarm_signal = (rand() % 6 == 0);  // ~17% chance
    system_on = (rand() % 2 == 0);
}

/**
 * @brief Logs the controller state.
 */
void log_hvac_state(const char* reason, int position) {
    printf(
        "Logic: %-28s | Damper: %3d%% | FireConfirmed: %s | System: %s\n",
        reason,
        position,
        fire_alarm_confirmed ? "YES" : "NO",
        system_on ? "ON" : "OFF"
    );
}

/**
 * @brief Clamps a value to a given range.
 */
int clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/**
 * @brief Main HVAC damper control logic.
 */
int step(int last_damper_position) {
    int desired_position = last_damper_position;

    // --- Fire alarm confirmation logic ---
    if (fire_alarm_signal) {
        fire_alarm_counter++;
        if (fire_alarm_counter >= FIRE_PREALARM_COUNT) {
            fire_alarm_confirmed = true;
        }
    } else {
        fire_alarm_counter = 0;
        fire_alarm_confirmed = false;
    }

    // --- 1. CRITICAL SAFETY OVERRIDE ---
    if (fire_alarm_confirmed) {
        desired_position = DAMPER_FIRE_SAFE_POSITION;
        log_hvac_state("FIRE ALARM CONFIRMED", desired_position);
    }
    // --- 2. SYSTEM OFF OVERRIDE ---
    else if (!system_on) {
        desired_position = DAMPER_MIN_OPEN;
        log_hvac_state("SYSTEM OFF OVERRIDE", desired_position);
    }
    // --- 3. NORMAL OPERATION ---
    else {
        float temp_error = current_temp_f - target_temp_f;

        if (fabs(temp_error) <= TEMP_DEADBAND) {
            desired_position = last_damper_position; // Hold
            log_hvac_state("TEMP WITHIN DEADBAND", desired_position);
        } else if (temp_error > 0) {
            // Too hot → open damper proportionally
            desired_position = clamp(
                last_damper_position + (int)(temp_error * 5),
                DAMPER_MIN_OPEN,
                DAMPER_MAX_OPEN
            );
            log_hvac_state("COOLING MODULATION", desired_position);
        } else {
            // Too cold → close damper gradually
            desired_position = clamp(
                last_damper_position - (int)(fabs(temp_error) * 5),
                DAMPER_MIN_OPEN,
                DAMPER_MAX_OPEN
            );
            log_hvac_state("CLOSING FOR HEAT BALANCE", desired_position);
        }
    }

    // --- 4. RATE LIMITING (MECHANICAL SAFETY) ---
    int delta = desired_position - last_damper_position;
    if (delta > MAX_STEP) delta = MAX_STEP;
    if (delta < -MAX_STEP) delta = -MAX_STEP;

    int new_position = last_damper_position + delta;

    // --- 5. FINAL HARD SATURATION ---
    new_position = clamp(new_position, DAMPER_MIN_OPEN, DAMPER_MAX_OPEN);

    return new_position;
}

/**
 * @brief Main execution loop.
 */
int main() {
    int damper_position = 0;
    int last_damper_position = 0;

    printf("--- Advanced HVAC Damper Controller Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_hvac_sensors();
        last_damper_position = damper_position;
        damper_position = step(damper_position);
    }

    return 0;
}

