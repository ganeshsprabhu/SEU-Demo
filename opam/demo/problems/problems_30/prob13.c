#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

/* SAFETY CONDITION:
 *
        (MIN_THROTTLE <= motor_throttle && motor_throttle <= MAX_THROTTLE)
 */

// --- Constants and Definitions ---
#define MAX_THROTTLE 1000
#define MIN_THROTTLE 0

#define BATTERY_WARNING_VOLTAGE 3500   // mV
#define BATTERY_CRITICAL_VOLTAGE 3300  // mV
#define BATTERY_CUTOFF_VOLTAGE 3200    // mV

#define RTL_THROTTLE_COMMAND 400
#define MAX_THROTTLE_STEP 50           // Rate limit per control step

#define FLIGHT_MODE_ACRO  0
#define FLIGHT_MODE_ANGLE 1

// --- Simulated Hardware Inputs ---
volatile int pilot_throttle_input;  
volatile int battery_voltage_mv;
volatile int flight_mode;            // CRV candidate
volatile bool armed;

// --- Internal Controller State ---
static int rtl_hold_counter = 0;

// --- Function Prototypes ---
void read_drone_sensors();
void log_drone_state(const char* reason, int throttle);
int apply_rate_limit(int last, int desired);
int step(int last_throttle_cmd);

/**
 * @brief Simulates reading drone sensors.
 */
void read_drone_sensors() {
    pilot_throttle_input = rand() % (MAX_THROTTLE + 1);
    battery_voltage_mv   = 3200 + rand() % 801;
    flight_mode          = rand() % 2;
    armed                = rand() % 2;
}

/**
 * @brief Limits how fast throttle can change between control steps.
 */
int apply_rate_limit(int last, int desired) {
    if (desired > last + MAX_THROTTLE_STEP)
        return last + MAX_THROTTLE_STEP;
    if (desired < last - MAX_THROTTLE_STEP)
        return last - MAX_THROTTLE_STEP;
    return desired;
}

/**
 * @brief Logs controller decisions.
 */
void log_drone_state(const char* reason, int throttle) {
    printf("Logic: %-30s | Throttle: %4d | Battery: %4dmV\n",
           reason, throttle, battery_voltage_mv);
}

/**
 * @brief Main drone motor control logic.
 */
int step(int last_throttle_cmd) {
    int desired_throttle = last_throttle_cmd;

    // 1. HARD SAFETY CUTOFF: Battery critically depleted
    if (battery_voltage_mv <= BATTERY_CUTOFF_VOLTAGE) {
        desired_throttle = 0;
        log_drone_state("BATTERY CUTOFF", desired_throttle);
    }

    // 2. CRITICAL FAILSAFE: Return-to-Land (CRV fully ignored)
    else if (battery_voltage_mv < BATTERY_CRITICAL_VOLTAGE) {
        desired_throttle = RTL_THROTTLE_COMMAND;
        rtl_hold_counter = 10; // Hold RTL for N cycles
        log_drone_state("CRITICAL BATTERY → RTL", desired_throttle);
    }

    // 3. POST-RTL HYSTERESIS (prevents oscillation)
    else if (rtl_hold_counter > 0) {
        rtl_hold_counter--;
        desired_throttle = RTL_THROTTLE_COMMAND;
        log_drone_state("RTL HOLD", desired_throttle);
    }

    // 4. STANDARD OPERATION
    else if (armed) {
        if (flight_mode == FLIGHT_MODE_ACRO) {
            desired_throttle = pilot_throttle_input;
            log_drone_state("ACRO MODE", desired_throttle);
        } else {
            desired_throttle = (last_throttle_cmd + pilot_throttle_input) / 2;
            log_drone_state("ANGLE MODE (SMOOTHED)", desired_throttle);
        }

        // Battery warning derates max throttle
        if (battery_voltage_mv < BATTERY_WARNING_VOLTAGE) {
            desired_throttle = desired_throttle * 70 / 100;
            log_drone_state("BATTERY WARNING DERATE", desired_throttle);
        }
    }

    // 5. DISARMED STATE
    else {
        desired_throttle = 0;
        log_drone_state("DISARMED", desired_throttle);
    }

    // 6. RATE LIMITING
    desired_throttle = apply_rate_limit(last_throttle_cmd, desired_throttle);

    // 7. FINAL SAFETY SATURATION
    if (desired_throttle > MAX_THROTTLE) desired_throttle = MAX_THROTTLE;
    if (desired_throttle < MIN_THROTTLE) desired_throttle = MIN_THROTTLE;

    // 8. COMPOUND SAFETY INVARIANT (complex-seeming)
    // If battery is critical OR drone is disarmed,
    // throttle must never increase relative to the last command.
    if ((battery_voltage_mv < BATTERY_CRITICAL_VOLTAGE || !armed) &&
        desired_throttle > last_throttle_cmd) {
        desired_throttle = last_throttle_cmd;
        log_drone_state("SAFETY INVARIANT ENFORCED", desired_throttle);
    }

    return desired_throttle;
}

/**
 * @brief Main execution loop.
 */
int main() {
    int motor_throttle = 0;

    printf("--- Refined Drone Motor Control Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_drone_sensors();
        motor_throttle = step(motor_throttle);
    }
    return 0;
}

