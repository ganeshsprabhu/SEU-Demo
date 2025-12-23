#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* SAFETY CONDITION:
 *	(commanded_angle >= MIN_ANGLE && commanded_angle <= MAX_ANGLE)
 * &&
 * 	(!(battery_overheat || actuator_fault) || commanded_angle == SAFE_MODE_ANGLE)
 */

// --- Constants and Definitions ---
#define MAX_ANGLE 90
#define MIN_ANGLE -90
#define SAFE_MODE_ANGLE 90          // Off-sun orientation
#define HIGH_BATTERY_TEMP_THRESHOLD 60  // Celsius
#define SENSOR_DISAGREE_THRESHOLD 15    // Degrees
#define MAX_SLEW_RATE 10                // Degrees per step

// --- Simulated Hardware Inputs ---
volatile int battery_temperature;
volatile int sun_position_angle;        // Primary ephemeris (CRV candidate)
volatile int backup_sun_sensor_angle;   // Redundant sensor
volatile bool actuator_fault;
volatile bool positioner_enabled;

// --- Function Prototypes ---
void read_satellite_sensors();
void log_array_angle(const char* reason, int angle);
int saturate(int value, int min, int max);
int apply_slew_rate(int desired, int current);

// --- Sensor Simulation ---
void read_satellite_sensors() {
    battery_temperature = 45 + (rand() % 25);           // 45–69 °C
    sun_position_angle = (rand() % 181) - 90;           // -90 to +90
    backup_sun_sensor_angle = sun_position_angle + ((rand() % 11) - 5);
    actuator_fault = (rand() % 20 == 0);                // 5% fault chance
    positioner_enabled = (rand() % 5 != 0);             // Mostly enabled
}

// --- Logging ---
void log_array_angle(const char* reason, int angle) {
    printf("Logic: %-30s | Array Angle: %d°\n", reason, angle);
}

// --- Utility ---
int saturate(int value, int min, int max) {
    if (value > max) return max;
    if (value < min) return min;
    return value;
}

int apply_slew_rate(int desired, int current) {
    if (desired > current + MAX_SLEW_RATE)
        return current + MAX_SLEW_RATE;
    if (desired < current - MAX_SLEW_RATE)
        return current - MAX_SLEW_RATE;
    return desired;
}

// --- Main Control Logic ---
int step_control_logic(int last_angle) {
    int commanded_angle = last_angle;
    bool battery_overheat = battery_temperature > HIGH_BATTERY_TEMP_THRESHOLD;
    bool sun_sensor_disagree =
        abs(sun_position_angle - backup_sun_sensor_angle) > SENSOR_DISAGREE_THRESHOLD;

    // 1. CRITICAL SAFETY OVERRIDE: Thermal or Actuator Fault
    // Makes ephemeris and enable state irrelevant.
    if (battery_overheat || actuator_fault) {
        commanded_angle = SAFE_MODE_ANGLE;
        log_array_angle("SAFE MODE (THERMAL/FAULT)", commanded_angle);
    }
    // 2. SECONDARY SAFETY: Sensor Disagreement
    // Degrades to conservative half-angle strategy.
    else if (sun_sensor_disagree) {
        commanded_angle = sun_position_angle / 2;
        log_array_angle("SENSOR DISAGREEMENT DEGRADED", commanded_angle);
    }
    // 3. STANDARD OPERATIONAL LOGIC
    else if (positioner_enabled) {
        commanded_angle = sun_position_angle;
        log_array_angle("NORMAL SUN TRACKING", commanded_angle);
    }
    // 4. POSITIONER DISABLED
    else {
        commanded_angle = last_angle;
        log_array_angle("POSITIONER HOLD", commanded_angle);
    }

    // Slew-rate limiting and saturation
    commanded_angle = apply_slew_rate(commanded_angle, last_angle);
    commanded_angle = saturate(commanded_angle, MIN_ANGLE, MAX_ANGLE);

    return commanded_angle;
}

// --- Main Execution Loop ---
int main() {
    srand(time(NULL));
    int array_angle = 0;

    printf("--- Satellite Solar Array Positioner Simulation ---\n");

    for (int i = 0; i < 50; ++i) {
        read_satellite_sensors();
        array_angle = step_control_logic(array_angle);
    }

    return 0;
}

