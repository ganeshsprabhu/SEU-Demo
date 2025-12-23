#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
/* SAFETY CONDITION:
 *
 * 1. Signal must be within valid domain
 * 2. GREEN is forbidden if:
 *      - track is occupied
 *      - communications have failed
 *      - sensors are unhealthy
 *      - no scheduled train is due
 * 3. Time must always remain within bounds
*/
/* SAFETY CONDITION (CODE):
 *	(signal_state == STATE_RED || signal_state == STATE_YELLOW || signal_state == STATE_GREEN)
 * &&
 * 	!(signal_state == STATE_GREEN && (track_occupied || !comms_link_ok || !sensor_health_ok || !scheduled_train_due))
 * &&
 * 	(time_of_day >= MIN_TIME && time_of_day <= MAX_TIME)
 */

// --- Constants and Definitions ---
#define STATE_RED     0
#define STATE_YELLOW  1
#define STATE_GREEN   2

#define MIN_TIME 0
#define MAX_TIME 1440   // minutes in a day

// --- Simulated Hardware Inputs ---
volatile bool track_occupied;          // Track circuit / axle counter
volatile int  time_of_day;             // Minutes from midnight
volatile bool scheduled_train_due;     // Timetable (CRV candidate)
volatile bool comms_link_ok;            // Signal interlocking link
volatile bool sensor_health_ok;         // Self-diagnostic flag

// --- Function Prototypes ---
void read_railway_sensors(void);
void log_signal_state(const char* reason, int state);
const char* state_to_string(int state);
int step(int last_signal_state);

/**
 * @brief Simulates reading data from railway field sensors.
 */
void read_railway_sensors(void) {
    track_occupied      = (rand() % 3 == 0);         // ~33% chance occupied
    time_of_day         = 480 + rand() % 30;         // 08:00–08:29
    scheduled_train_due = (time_of_day >= 485 && time_of_day <= 495);
    comms_link_ok       = (rand() % 10 != 0);        // 90% reliable
    sensor_health_ok    = (rand() % 20 != 0);        // 95% healthy
}

/**
 * @brief Logs the signal state for debugging.
 */
void log_signal_state(const char* reason, int state) {
    printf(
        "Logic: %-28s | Signal: %-6s | Occupied: %s | Comms: %s\n",
        reason,
        state_to_string(state),
        track_occupied ? "YES" : "NO",
        comms_link_ok ? "OK" : "FAIL"
    );
}

const char* state_to_string(int state) {
    if (state == STATE_RED)    return "RED";
    if (state == STATE_YELLOW) return "YELLOW";
    if (state == STATE_GREEN)  return "GREEN";
    return "INVALID";
}

/**
 * @brief Main signal control logic.
 */
int step(int last_signal_state) {
    int new_state = STATE_RED;

    // --- SAFETY OVERRIDE LAYER ---
    if (!comms_link_ok) {
        new_state = STATE_RED;
        log_signal_state("FAILSAFE: COMMS LOST", new_state);
    }
    else if (!sensor_health_ok) {
        new_state = STATE_RED;
        log_signal_state("FAILSAFE: SENSOR FAULT", new_state);
    }
    else if (track_occupied && !scheduled_train_due) {
        new_state = STATE_RED;
        log_signal_state("UNSCHEDULED OCCUPANCY", new_state);
    }

    // --- CONTROLLED TRANSITION LOGIC ---
    else {
        if (scheduled_train_due && !track_occupied) {
            if (last_signal_state == STATE_RED) {
                new_state = STATE_YELLOW;
                log_signal_state("PREPARE ROUTE", new_state);
            } else {
                new_state = STATE_GREEN;
                log_signal_state("SCHEDULED PASSAGE", new_state);
            }
        } else {
            new_state = STATE_RED;
            log_signal_state("NO TRAIN AUTHORITY", new_state);
        }
    }

    return new_state;
}

/**
 * @brief Main execution loop with explicit safety condition.
 */
int main(void) {
    srand(time(NULL));
    int signal_state = STATE_RED;

    printf("--- Advanced Railway Signal Control Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_railway_sensors();
        signal_state = step(signal_state);

    }

    return 0;
}

