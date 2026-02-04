#include <stdio.h>
#include <stdbool.h>

// Function to simulate a room temperature control system using integers
int control_temperature(int temperature, int fan_speed, int mode, int humidity_level) {
    bool alarm = false;

    // Temperature adjustment based on mode
    if (mode == 1) {  // Cooling mode
        temperature -= fan_speed;
    } else if (mode == 2) {  // Heating mode
        temperature += fan_speed;
    } else {  // Invalid mode
        alarm = true;
        printf("Invalid mode! Setting alarm...\n");
    }

    // Adjust temperature based on humidity, using integer math
    if (humidity_level >= 20 && humidity_level <= 80) {
        // humidity effect = humidity * 5 / 100 = humidity * 0.05
        int humidity_effect = (humidity_level * 5) / 10;  // Scaled to match tenths of a degree
        temperature += humidity_effect;
    }

    // Safety property: Temperature must not exceed 300 (i.e., 30.0°C)
    if (temperature > 300) {
        alarm = true;
    }

    printf("Alarm: %s\n", alarm ? "ON" : "OFF");
    return temperature;
}

int main() {
    int temperature = 250;       // Initial room temperature: 25.0°C
    int fan_speed = 5;           // Fan speed in tenths of degree
    int mode = 1;                // Mode: 1 for cooling, 2 for heating
    int humidity_level = 60;     // Humidity level (affects comfort but not directly safety)

    int final_temperature = control_temperature(temperature, fan_speed, mode, humidity_level);
    printf("Final temperature: %d.%d°C\n", final_temperature / 10, final_temperature % 10);
    return 0;
}
