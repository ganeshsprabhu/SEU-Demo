#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 *	(press_state == STATE_RUNNING) 
 * &&
 * 	(light_curtain_breached)
 */

#define STATE_STOPPED 0
#define STATE_RUNNING 1
#define BATCH_SIZE 1000

// Simulated hardware inputs
volatile bool light_curtain_breached;
volatile int part_counter; // (CRV candidate)
volatile bool start_command;

void read_press_sensors() {
    light_curtain_breached = (rand() % 20 == 0); // 5% chance of breach
    start_command = true;
    // part_counter is incremented in the loop
}

void log_press_state(const char* reason, int state) {
    printf("Reason: %-20s | Press State: %s\n", reason, state == STATE_RUNNING ? "RUNNING" : "STOPPED");
}

int step_control_logic(int current_state) {
    int new_state = current_state;

    // 1. Critical Safety Override: Light Curtain
    if (light_curtain_breached) {
        new_state = STATE_STOPPED;
        log_press_state("LIGHT CURTAIN FAULT", new_state);
    } 
    // 2. Standard Operational Logic
    else {
        // Logic depends on the CRV 'part_counter'
        if (current_state == STATE_STOPPED && start_command) {
            if (part_counter < BATCH_SIZE) {
                new_state = STATE_RUNNING;
                log_press_state("Start Command", new_state);
            } else {
                log_press_state("Batch Complete", STATE_STOPPED);
            }
        } else if (current_state == STATE_RUNNING) {
            if (part_counter >= BATCH_SIZE) {
                 new_state = STATE_STOPPED;
                 log_press_state("Batch Size Reached", new_state);
            } else {
                new_state = STATE_RUNNING; // Continue running
                part_counter++;
            }
        }
    }
    return new_state;
}

int main() {
    srand(time(NULL));
    int press_state = STATE_STOPPED;
    part_counter = 995; // Start near the end of the batch
    printf("--- Industrial Press Control Simulation (Single Function) ---\n");

    for (int i = 0; i < 10; ++i) {
        read_press_sensors();
        press_state = step_control_logic(press_state);
        
    }
    return 0;
}

