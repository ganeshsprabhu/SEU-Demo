#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 *	(methane_level_ppm > METHANE_THRESHOLD && fan_speed == FAN_SPEED_MAX)
 * && 
 * 	(high_methane_counter <= HIGH_METHANE_DURATION);
 */

#define FAN_SPEED_MAX 100
#define FAN_SPEED_NORMAL 60
#define FAN_SPEED_IDLE 20
#define METHANE_THRESHOLD 500   // ppm
#define HIGH_METHANE_DURATION 3 // consecutive readings

// Simulated hardware inputs
volatile int methane_level_ppm;
volatile bool is_work_shift_active; // CRV candidate

// Safety tracking
volatile int high_methane_counter = 0;

// Function prototypes
void read_mine_sensors();
void log_fan_speed(const char* reason, int speed);
int step_control_logic(int last_fan_speed);
bool check_safety(int fan_speed);

void read_mine_sensors() {
    methane_level_ppm = 400 + (rand() % 300); // 400-699 ppm
    is_work_shift_active = (rand() % 2 == 0);
}

void log_fan_speed(const char* reason, int speed) {
    printf("Reason: %-25s | Fan Speed: %d%% | Methane: %d ppm | HighMethaneCnt: %d\n",
           reason, speed, methane_level_ppm, high_methane_counter);
}

int step_control_logic(int last_fan_speed) {
    int new_fan_speed = last_fan_speed;

    // 1. Critical Safety Override: High Methane
    if (methane_level_ppm > METHANE_THRESHOLD) {
        high_methane_counter++;
        new_fan_speed = FAN_SPEED_MAX;
        log_fan_speed("HIGH METHANE - PURGE", new_fan_speed);
    } else {
        high_methane_counter = 0;
        // 2. Standard Operational Logic
        if (is_work_shift_active) {
            // Gradually ramp up/down for comfort
            if (last_fan_speed < FAN_SPEED_NORMAL)
                new_fan_speed = last_fan_speed + 5 > FAN_SPEED_NORMAL ? FAN_SPEED_NORMAL : last_fan_speed + 5;
            else if (last_fan_speed > FAN_SPEED_NORMAL)
                new_fan_speed = last_fan_speed - 5 < FAN_SPEED_NORMAL ? FAN_SPEED_NORMAL : last_fan_speed - 5;
            log_fan_speed("Work Shift Active", new_fan_speed);
        } else {
            if (last_fan_speed < FAN_SPEED_IDLE)
                new_fan_speed = last_fan_speed + 2 > FAN_SPEED_IDLE ? FAN_SPEED_IDLE : last_fan_speed + 2;
            else if (last_fan_speed > FAN_SPEED_IDLE)
                new_fan_speed = last_fan_speed - 2 < FAN_SPEED_IDLE ? FAN_SPEED_IDLE : last_fan_speed - 2;
            log_fan_speed("Off-Shift Idle", new_fan_speed);
        }
    }

    // 3. Final saturation
    if (new_fan_speed > FAN_SPEED_MAX) new_fan_speed = FAN_SPEED_MAX;
    if (new_fan_speed < 0) new_fan_speed = 0;

    return new_fan_speed;
}

int main() {
    srand(time(NULL));
    int fan_speed = FAN_SPEED_IDLE;
    printf("--- Mining Ventilation Control Simulation (Refined) ---\n");

    for (int i = 0; i < 50; i++) {
        read_mine_sensors();
        fan_speed = step_control_logic(fan_speed);
    }

    return 0;
}

