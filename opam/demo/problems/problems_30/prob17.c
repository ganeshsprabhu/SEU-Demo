// SAFETY PROPERTY:
// gas_valve_state == OPEN is allowed ONLY IF:
//   thermostat_calls_for_heat == true AND
//   lockout_active == false AND
//   (flame_detected == true OR time_since_valve_open <= IGNITION_TIMEOUT) AND
//   ignition_attempts <= MAX_IGNITION_ATTEMPTS

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION EXPLANATION:
 * The gas valve is allowed to be OPEN if and only if:
	The thermostat is actively requesting heat (CRV),
	The controller is not in lockout,
	Ignition retry count is within safe bounds,
	AND either:
		A flame is detected, or
		The ignition timeout window has not yet expired.
   Otherwise, the valve must be closed.
*/

/* SAFETY CONDITION IN CODE:
 *	(gas_valve_state == VALVE_CLOSED)
 * ||
 * 	(
		thermostat_calls_for_heat &&
		!lockout_active &&
		ignition_attempts <= MAX_IGNITION_ATTEMPTS &&
		(
		    flame_detected ||
		    time_since_valve_open <= IGNITION_TIMEOUT
		)
	)
 */

#define VALVE_CLOSED 0
#define VALVE_OPEN   1

#define IGNITION_TIMEOUT 5        // seconds
#define PREPURGE_TIME 3           // seconds
#define MAX_IGNITION_ATTEMPTS 3

// --- Simulated hardware inputs ---
volatile bool thermostat_calls_for_heat;   // CRV candidate
volatile bool flame_detected;
volatile int time_since_valve_open;

// --- Internal controller state ---
volatile int ignition_attempts;
volatile bool lockout_active;
volatile int prepurge_timer;

// --- Sensor simulation ---
void read_furnace_sensors(bool valve_is_open) {
    thermostat_calls_for_heat = true; // Assume continuous heat demand

    if (valve_is_open) {
        // Flame may fail occasionally
        flame_detected = (rand() % 10 != 0); // 90% success
    } else {
        flame_detected = false;
    }
}

void log_state(const char* reason, int valve_state) {
    printf(
        "Logic: %-30s | Valve: %-6s | Flame: %-3s | Attempts: %d | Lockout: %s\n",
        reason,
        valve_state == VALVE_OPEN ? "OPEN" : "CLOSED",
        flame_detected ? "YES" : "NO",
        ignition_attempts,
        lockout_active ? "YES" : "NO"
    );
}

// --- Control logic ---
int step_control_logic(int current_valve_state) {
    int new_valve_state = current_valve_state;

    /* ================================
       HARD SAFETY LOCKOUT CHECK
       ================================ */
    if (lockout_active) {
        new_valve_state = VALVE_CLOSED;
        log_state("LOCKOUT ACTIVE", new_valve_state);
        return new_valve_state;
    }

    /* ================================
       VALVE OPEN STATE
       ================================ */
    if (current_valve_state == VALVE_OPEN) {
        time_since_valve_open++;

        // Critical safety override: ignition failure
        if (!flame_detected && time_since_valve_open > IGNITION_TIMEOUT) {
            ignition_attempts++;
            new_valve_state = VALVE_CLOSED;
            log_state("IGNITION FAILURE - VALVE CLOSED", new_valve_state);

            if (ignition_attempts >= MAX_IGNITION_ATTEMPTS) {
                lockout_active = true;
                log_state("MAX ATTEMPTS REACHED - LOCKOUT", new_valve_state);
            }
        } else {
            log_state("FLAME STABLE", VALVE_OPEN);
        }
    }

    /* ================================
       VALVE CLOSED STATE
       ================================ */
    else {
        if (thermostat_calls_for_heat) {
            // Pre-purge phase before ignition
            if (prepurge_timer < PREPURGE_TIME) {
                prepurge_timer++;
                log_state("PRE-PURGE IN PROGRESS", VALVE_CLOSED);
            } else {
                new_valve_state = VALVE_OPEN;
                time_since_valve_open = 0;
                prepurge_timer = 0;
                log_state("IGNITION ATTEMPT", new_valve_state);
            }
        } else {
            log_state("NO HEAT DEMAND", VALVE_CLOSED);
        }
    }

    /* ================================
       NON-TRIVIAL SAFETY CONDITION
       ================================ */
    bool safety_ok =
        (new_valve_state == VALVE_CLOSED) ||
        (
            thermostat_calls_for_heat &&
            !lockout_active &&
            ignition_attempts <= MAX_IGNITION_ATTEMPTS &&
            (flame_detected || time_since_valve_open <= IGNITION_TIMEOUT)
        );

    if (!safety_ok) {
        printf("!!! SAFETY VIOLATION: GAS VALVE FORCED CLOSED !!!\n");
        new_valve_state = VALVE_CLOSED;
        lockout_active = true;
    }

    return new_valve_state;
}

// --- Main loop ---
int main() {
    srand(time(NULL));

    int gas_valve_state = VALVE_CLOSED;
    time_since_valve_open = 0;
    ignition_attempts = 0;
    lockout_active = false;
    prepurge_timer = 0;

    printf("--- Advanced Furnace Ignition Controller Simulation ---\n\n");

    for (int i = 0; i < 30; i++) {
        read_furnace_sensors(gas_valve_state == VALVE_OPEN);
        gas_valve_state = step_control_logic(gas_valve_state);

    }
    return 0;
}

