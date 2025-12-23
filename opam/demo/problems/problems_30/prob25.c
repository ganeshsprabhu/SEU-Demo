// robotic_arm_controller.c
// Advanced control logic for a robotic arm motor. The controller selects torque based on
// position error, speed profile, and motion enablement. Multiple safety overrides (collision,
// sensor plausibility, and rate limiting) enforce a complex safety invariant that constrains
// torque magnitude, direction, and transition behavior.
//
// SAFETY INVARIANT (informal):
// - Motor torque magnitude must never exceed MAX_MOTOR_TORQUE
// - If a collision is detected, torque must be exactly zero
// - If motion is disabled, torque must be zero
// - Torque direction must oppose position error
// - Torque changes must respect a maximum slew rate
//
// motor_torque_command ∈ [-MAX_MOTOR_TORQUE, MAX_MOTOR_TORQUE]

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

/* SAFETY CONDITION:
 *	(motor_torque >= -MAX_MOTOR_TORQUE && motor_torque <=  MAX_MOTOR_TORQUE)
 * &&
 * 	(!(force_sensor_n > COLLISION_FORCE_N) || motor_torque == 0)
 * &&
 * 	(!motion_enabled || motor_torque == 0 || abs(target_position - current_position) > POSITION_TOLERANCE)
 * &&
 * 	(motor_torque == 0 || ((target_position - current_position > 0) && motor_torque > 0) || ((target_position - current_position < 0) && motor_torque < 0))
 */

// --- Constants and Definitions ---
#define MAX_MOTOR_TORQUE        100
#define MAX_TORQUE_SLEW         20    // Max change per step
#define COLLISION_FORCE_N       50.0f
#define POSITION_TOLERANCE      5
#define TORQUE_PROFILE_FAST     80
#define TORQUE_PROFILE_PRECISE  30
#define SENSOR_FORCE_MAX        500.0f

// --- Simulated Hardware Inputs ---
volatile int target_position;
volatile int current_position;
volatile float force_sensor_n;
volatile int speed_profile;   // 0: Precise, 1: Fast (CRV candidate)
volatile bool motion_enabled;

// --- Function Prototypes ---
void read_robot_sensors(void);
void log_robot_state(const char* reason, int torque);
int clamp(int value, int min, int max);
int apply_slew_rate(int last, int target);
int step(int last_torque);

// --- Sensor Simulation ---
void read_robot_sensors(void) {
    target_position  = 900 + rand() % 201;     // 900–1100
    current_position = 400 + rand() % 201;     // 400–600
    speed_profile    = rand() % 2;
    motion_enabled   = rand() % 2;
    force_sensor_n   = (float)(rand() % 120);  // 0–119 N
}

// --- Utilities ---
void log_robot_state(const char* reason, int torque) {
    printf("Logic: %-30s | Torque Command: %4d\n", reason, torque);
}

int clamp(int value, int min, int max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int apply_slew_rate(int last, int target) {
    if (target > last + MAX_TORQUE_SLEW) return last + MAX_TORQUE_SLEW;
    if (target < last - MAX_TORQUE_SLEW) return last - MAX_TORQUE_SLEW;
    return target;
}

// --- Main Control Logic ---
int step(int last_torque) {
    int new_torque = 0;
    int position_error = target_position - current_position;

    // 1. CRITICAL SAFETY OVERRIDE: Sensor Plausibility
    if (force_sensor_n < 0.0f || force_sensor_n > SENSOR_FORCE_MAX) {
        new_torque = 0;
        log_robot_state("FORCE SENSOR FAULT", new_torque);
    }
    // 2. CRITICAL SAFETY OVERRIDE: Collision Detection
    else if (force_sensor_n > COLLISION_FORCE_N) {
        new_torque = 0;
        log_robot_state("COLLISION DETECTED", new_torque);
    }
    // 3. MOTION DISABLED OR AT TARGET
    else if (!motion_enabled || abs(position_error) <= POSITION_TOLERANCE) {
        new_torque = 0;
        log_robot_state("MOTION DISABLED / AT TARGET", new_torque);
    }
    // 4. NORMAL OPERATIONAL CONTROL
    else {
        int base_torque =
            (speed_profile == 0) ? TORQUE_PROFILE_PRECISE : TORQUE_PROFILE_FAST;

        // Direction must oppose position error
        int desired_torque =
            (position_error > 0) ? base_torque : -base_torque;

        // Apply slew-rate limiting
        new_torque = apply_slew_rate(last_torque, desired_torque);

        log_robot_state(
            (speed_profile == 0) ? "PRECISE PROFILE CONTROL" : "FAST PROFILE CONTROL",
            new_torque
        );
    }

    // 5. FINAL SATURATION
    new_torque = clamp(new_torque, -MAX_MOTOR_TORQUE, MAX_MOTOR_TORQUE);

    return new_torque;
}

// --- Main Execution Loop ---
int main(void) {
    int motor_torque = 0;

    printf("--- Robotic Arm Control Simulation (Refined Logic) ---\n");

    for (int i = 0; i < 200; ++i) {
        read_robot_sensors();
        motor_torque = step(motor_torque);

    }

    return 0;
}

