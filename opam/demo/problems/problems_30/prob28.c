#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/*
 * SAFETY CONDITION
 * 1. Pump action must be in valid domain
 * 2. If pressure exceeds crush depth, pump must be EMPTY
 * 3. If depth exceeds max safe depth, pump cannot be FILL
 * 4. If pressure is safe, HOLD or controlled motion is allowed
 */

/* SAFETY CONDITION (CODE):
 * 	(pump_action >= -1 && pump_action <= 1)
 * &&
 * 	(!(hull_pressure >= CRUSH_DEPTH_PRESSURE) || (pump_action == PUMP_ACTION_EMPTY))
 * &&
 *	(!(current_depth >= MAX_SAFE_DEPTH) || (pump_action != PUMP_ACTION_FILL))
 */

// --- Constants and Definitions ---
#define PUMP_ACTION_FILL   1    // Increase depth
#define PUMP_ACTION_EMPTY -1    // Decrease depth / surface
#define PUMP_ACTION_HOLD   0

#define CRUSH_DEPTH_PRESSURE 1000   // PSI
#define WARNING_PRESSURE     900    // PSI
#define MAX_SAFE_DEPTH       120    // meters
#define MAX_DEPTH_RATE       5      // meters per step (simulated)

// --- Simulated Hardware Inputs ---
volatile int hull_pressure;     // PSI
volatile int current_depth;     // meters
volatile int previous_depth;    // meters
volatile int target_depth;      // CRV candidate

// --- Function Prototypes ---
void read_sub_sensors();
void log_pump_action(const char* reason, int action);
int step_control_logic();

// --- Sensor Simulation ---
void read_sub_sensors() {
    hull_pressure = 800 + (rand() % 300);   // 800–1100 PSI
    previous_depth = current_depth;
    current_depth = hull_pressure / 10;    // Simplified relationship
    target_depth = 120;                    // Dive command (CRV)
}

// --- Logging ---
void log_pump_action(const char* reason, int action) {
    const char* action_str = "HOLD";
    if (action == PUMP_ACTION_FILL)  action_str = "FILLING (DIVE)";
    if (action == PUMP_ACTION_EMPTY) action_str = "EMPTYING (SURFACE)";
    printf("Reason: %-30s | Pump Action: %s | Depth: %dm | Pressure: %d PSI\n",
           reason, action_str, current_depth, hull_pressure);
}

// --- Main Control Logic ---
int step_control_logic() {
    int pump_action = PUMP_ACTION_HOLD;
    int depth_rate = current_depth - previous_depth;

    // 1. HARD SAFETY OVERRIDE: Crush depth imminent
    // CRV 'target_depth' is irrelevant here
    if (hull_pressure >= CRUSH_DEPTH_PRESSURE) {
        pump_action = PUMP_ACTION_EMPTY;
        log_pump_action("CRUSH DEPTH EMERGENCY", pump_action);
    }
    // 2. PRE-EMPTIVE SAFETY: Rapid descent near limit
    else if (hull_pressure >= WARNING_PRESSURE &&
             depth_rate > MAX_DEPTH_RATE) {
        pump_action = PUMP_ACTION_EMPTY;
        log_pump_action("RAPID DESCENT - PREVENTIVE SURFACE", pump_action);
    }
    // 3. STANDARD OPERATIONAL LOGIC
    else {
        // Logic depends on CRV 'target_depth'
        if (current_depth < target_depth) {
            pump_action = PUMP_ACTION_FILL;
            log_pump_action("DIVING TO TARGET DEPTH", pump_action);
        } else if (current_depth > target_depth) {
            pump_action = PUMP_ACTION_EMPTY;
            log_pump_action("ASCENDING TO TARGET DEPTH", pump_action);
        } else {
            pump_action = PUMP_ACTION_HOLD;
            log_pump_action("TARGET DEPTH MAINTAINED", pump_action);
        }
    }

    return pump_action;
}

int main() {
    srand(time(NULL));
    int pump_action = PUMP_ACTION_HOLD;
    current_depth = 80;
    previous_depth = 80;

    printf("--- Submarine Ballast Tank Control Simulation ---\n");

    for (int i = 0; i < 20; ++i) {
        read_sub_sensors();
        pump_action = step_control_logic();


    }

    return 0;
}

