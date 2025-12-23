#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>


/* SAFETY CONDITION:
 *	 (-MAX_DOOR_FORCE <= door_force && door_force <= MAX_DOOR_FORCE) && (!obstruction_detected || door_force >= 0)
 */

// --- Constants and Definitions ---
#define MAX_DOOR_FORCE        50
#define MAX_CLOSE_FORCE      -30
#define SOFT_CLOSE_FORCE     -15
#define DOOR_OPEN_FORCE       35
#define DOOR_HOLD_FORCE        0

#define LOBBY_FLOOR            1
#define OBSTRUCTION_HOLD_CYCLES 5

// --- Simulated Hardware Inputs ---
volatile bool obstruction_detected;
volatile bool command_close_door;
volatile int current_floor;
volatile bool special_hold_profile_active; // (CRV candidate)

// --- Internal State ---
static int hold_open_timer = 0;
static int obstruction_latch_timer = 0;

// --- Function Prototypes ---
void read_elevator_sensors();
void log_door_state(const char* reason, int force);
int step(int last_door_force);

/**
 * @brief Simulates reading data from elevator sensors.
 */
void read_elevator_sensors() {
    obstruction_detected = (rand() % 10 == 0);     // 10% obstruction chance
    command_close_door   = (rand() % 2 == 0);      // 50% close command
    current_floor        = 1 + (rand() % 5);       // Floors 1–5
    special_hold_profile_active = (current_floor == LOBBY_FLOOR);
}

/**
 * @brief Logs door motor output.
 */
void log_door_state(const char* reason, int force) {
    printf("Logic: %-30s | Door Motor Force: %d\n", reason, force);
}

/**
 * @brief Main elevator door control logic.
 */
int step(int last_door_force) {
    int new_force = DOOR_HOLD_FORCE;

    /* -------------------------------------------------
     * 1. CRITICAL SAFETY OVERRIDE: Obstruction Handling
     * -------------------------------------------------
     * Once triggered, obstruction forces doors open for
     * a minimum number of cycles (latch behavior).
     * All CRV logic and commands are ignored.
     */
    if (obstruction_detected) {
        obstruction_latch_timer = OBSTRUCTION_HOLD_CYCLES;
        new_force = DOOR_OPEN_FORCE;
        log_door_state("OBSTRUCTION DETECTED", new_force);
    }
    else if (obstruction_latch_timer > 0) {
        obstruction_latch_timer--;
        new_force = DOOR_OPEN_FORCE;
        log_door_state("OBSTRUCTION CLEARING WINDOW", new_force);
    }

    /* --------------------------------
     * 2. STANDARD OPERATIONAL LOGIC
     * -------------------------------- */
    else {
        // Door hold logic
        if (hold_open_timer > 0) {
            hold_open_timer--;
            new_force = DOOR_HOLD_FORCE;
            log_door_state("Holding Door Open", new_force);
        }
        // Closing logic
        else if (command_close_door) {
            // Soft-close if recently opening, else normal close
            if (last_door_force > 0) {
                new_force = SOFT_CLOSE_FORCE;
                log_door_state("Soft Close Transition", new_force);
            } else {
                new_force = MAX_CLOSE_FORCE;
                log_door_state("Closing Door", new_force);
            }
        }
        // Opening logic
        else {
            new_force = DOOR_OPEN_FORCE;

            // CRV-dependent hold time (ignored by safety logic)
            if (special_hold_profile_active) {
                hold_open_timer = 20;  // Lobby floor
                log_door_state("Opening (Lobby Profile)", new_force);
            } else {
                hold_open_timer = 6;   // Standard floors
                log_door_state("Opening (Standard Floor)", new_force);
            }
        }
    }

    /* --------------------------------
     * 3. COMPOUND SAFETY SATURATION
     * --------------------------------
     * Ensures:
     *  - Force is always bounded
     *  - Doors never close while obstruction is active
     */
    if (new_force > MAX_DOOR_FORCE) {
        new_force = MAX_DOOR_FORCE;
    }
    if (new_force < -MAX_DOOR_FORCE) {
        new_force = -MAX_DOOR_FORCE;
    }
    if (obstruction_detected && new_force < 0) {
        new_force = DOOR_OPEN_FORCE; // Absolute safety dominance
    }

    return new_force;
}

/**
 * @brief Main execution loop.
 */
int main() {
    int door_force = 0;
    srand(1);

    printf("--- Elevator Door Control Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_elevator_sensors();
        door_force = step(door_force);
    }

    return 0;
}

