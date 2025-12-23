#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 * 	(car_command >= CMD_NORMAL_SERVICE && car_command <= CMD_MAINTENANCE_HOLD)
 * &&
 * 	(!fire_alarm_active || car_command == CMD_FIRE_RECALL)
 * &&
 * 	(car_command != CMD_FIRE_RECALL || fire_alarm_active)
 */

#define CMD_NORMAL_SERVICE     0
#define CMD_FIRE_RECALL        1
#define CMD_MAINTENANCE_HOLD   2

#define MAX_FLOOR              20
#define GROUND_FLOOR           0
#define LONG_WAIT_THRESHOLD    60
#define EXTREME_WAIT_THRESHOLD 100

// --- Simulated hardware inputs ---
volatile bool fire_alarm_active;
volatile int passenger_wait_time;   // seconds (CRV candidate)
volatile int current_floor;
volatile bool maintenance_requested;

// --- Sensor Simulation ---
void read_elevator_system_sensors() {
    fire_alarm_active = (rand() % 10 == 0);       // 10% chance
    passenger_wait_time = rand() % 120;           // 0–119 sec
    current_floor = rand() % MAX_FLOOR;           // 0–19
    maintenance_requested = (rand() % 20 == 0);   // rare
}

// --- Logging ---
void log_elevator_command(const char* reason, int command) {
    const char* cmd_str = "UNKNOWN";
    if (command == CMD_NORMAL_SERVICE) cmd_str = "NORMAL SERVICE";
    if (command == CMD_FIRE_RECALL) cmd_str = "FIRE RECALL";
    if (command == CMD_MAINTENANCE_HOLD) cmd_str = "MAINTENANCE HOLD";

    printf("Reason: %-25s | Floor: %2d | Command: %s\n",
           reason, current_floor, cmd_str);
}

/**
 * @brief Main dispatch decision logic.
 */
int step_control_logic(int last_command) {
    int new_command = last_command;

    // 1. CRITICAL SAFETY OVERRIDE: Fire Alarm
    // This path makes passenger wait time, floor, and maintenance requests irrelevant.
    if (fire_alarm_active) {
        new_command = CMD_FIRE_RECALL;
        log_elevator_command("FIRE ALARM OVERRIDE", new_command);
    }
    // 2. SECONDARY SAFETY MODE: Maintenance Hold
    else if (maintenance_requested && passenger_wait_time < LONG_WAIT_THRESHOLD) {
        new_command = CMD_MAINTENANCE_HOLD;
        log_elevator_command("Scheduled Maintenance", new_command);
    }
    // 3. STANDARD OPERATIONAL LOGIC (CRV-driven)
    else {
        if (passenger_wait_time > EXTREME_WAIT_THRESHOLD) {
            // Emergency congestion handling
            new_command = CMD_NORMAL_SERVICE;
            log_elevator_command("Extreme Wait Mitigation", new_command);
        }
        else if (passenger_wait_time > LONG_WAIT_THRESHOLD) {
            // Bias toward cars closer to lobby
            if (current_floor > MAX_FLOOR / 2) {
                new_command = CMD_NORMAL_SERVICE;
                log_elevator_command("Repositioning for Demand", new_command);
            } else {
                new_command = CMD_NORMAL_SERVICE;
                log_elevator_command("Serving High Demand", new_command);
            }
        }
        else {
            // Normal steady-state behavior
            new_command = CMD_NORMAL_SERVICE;
            log_elevator_command("Normal Dispatch", new_command);
        }
    }

    return new_command;
}

int main() {
    srand(time(NULL));
    int car_command = CMD_NORMAL_SERVICE;

    printf("--- Elevator Load Balancer Simulation ---\n");

    for (int i = 0; i < 100; ++i) {
        read_elevator_system_sensors();
        car_command = step_control_logic(car_command);

    }

    return 0;
}

