#include <stdio.h>
#include <stdbool.h>

// Function to control fan speed based on conditions
int control_fan_speed(int temperature, int fan_speed, int system_mode, int operating_hours) {
    bool alarm = false;

    if (system_mode == 0) {  // Manual mode
        printf("Manual mode: Fan speed is fixed at %d.\n", fan_speed);
    } else if (system_mode == 1 && fan_speed < 90) {  // Auto mode, bounded
        if (temperature > 30) {
            fan_speed += 5;  
        } else if (temperature < 20) {
            fan_speed -= 5; 
        }
    }

    // Only apply operating_hours if current fan_speed is low (so it can't trigger alarm)
    if (operating_hours > 100 && fan_speed < 95) {
        fan_speed += 1;
    }

    // Safety property: Fan speed must not exceed 100
    if (fan_speed > 100) {
        alarm = true;
        printf("Alarm: Fan speed exceeded the safe limit!\n");
    } else {
        printf("Alarm: OFF\n");
    }

    return fan_speed;
}

int main() {
    int temperature = 35;        
    int fan_speed = 50; 
    int system_mode = 1;         // Automatic mode
    int operating_hours = 120;   // System usage

    int final_fan_speed = control_fan_speed(temperature, fan_speed, system_mode, operating_hours);
    printf("Final fan speed: %d\n", final_fan_speed);

    return 0;
}

