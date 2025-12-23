#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION:
 * 	(actuator_speed >= -MAX_ACTUATOR_SPEED && actuator_speed <=  MAX_ACTUATOR_SPEED)
 * &&
 * 	(!stow_latched ||
            (panel_angle_deg == STOW_POSITION_DEGREES ||
             (panel_angle_deg > STOW_POSITION_DEGREES && actuator_speed <= 0) ||
             (panel_angle_deg < STOW_POSITION_DEGREES && actuator_speed >= 0)))
 * &&
 * 	!(abs(ldr_east_lumens - ldr_west_lumens) > SENSOR_DISAGREE_LUMENS && actuator_speed != 0);
*/
 

// --- Constants and Definitions ---
#define MAX_ACTUATOR_SPEED       100
#define MAX_SLEW_RATE            20     // max speed change per step
#define WIND_SPEED_STOW_MPH      50
#define WIND_SPEED_RECOVER_MPH   40     // hysteresis
#define STOW_POSITION_DEGREES    0
#define MAX_PANEL_ANGLE          90
#define MIN_PANEL_ANGLE          0
#define LDR_THRESHOLD            50
#define SENSOR_DISAGREE_LUMENS   800

// --- Simulated Hardware Inputs ---
volatile int panel_angle_deg;
volatile int wind_speed_mph;
volatile int ldr_east_lumens;
volatile int ldr_west_lumens;
volatile int tracking_mode; // 0: LDR-based, 1: Predictive (CRV)
volatile bool system_enabled;

// --- Internal State ---
static bool stow_latched = false;

// --- Function Prototypes ---
void read_environmental_sensors(void);
void log_tracker_state(const char* reason, int speed);
int clamp(int v, int min, int max);
int apply_slew_limit(int desired, int last);
bool safety_condition(int actuator_speed);

// --- Sensor Simulation ---
void read_environmental_sensors() {
    panel_angle_deg    = rand() % 91;
    wind_speed_mph     = 10 + rand() % 71;
    ldr_east_lumens    = 500 + rand() % 1001;
    ldr_west_lumens    = 500 + rand() % 1001;
    tracking_mode      = rand() % 2;
    system_enabled     = rand() % 2;
}

// --- Logging ---
void log_tracker_state(const char* reason, int speed) {
    printf("Logic: %-30s | Speed: %4d | Angle: %3d | Wind: %2d\n",
           reason, speed, panel_angle_deg, wind_speed_mph);
}

// --- Utilities ---
int clamp(int v, int min, int max) {
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

int apply_slew_limit(int desired, int last) {
    if (desired > last + MAX_SLEW_RATE) return last + MAX_SLEW_RATE;
    if (desired < last - MAX_SLEW_RATE) return last - MAX_SLEW_RATE;
    return desired;
}

// --- Main Control Logic ---
int step(int last_actuator_speed) {
    int desired_speed = 0;

    // 1. SAFETY LATCH: Wind-based stow hysteresis
    if (wind_speed_mph > WIND_SPEED_STOW_MPH) {
        stow_latched = true;
    } else if (wind_speed_mph < WIND_SPEED_RECOVER_MPH) {
        stow_latched = false;
    }

    // 2. CRITICAL SAFETY OVERRIDE: Forced Stow
    // CRV: tracking_mode and light sensors are ignored
    if (stow_latched) {
        if (panel_angle_deg > STOW_POSITION_DEGREES)
            desired_speed = -MAX_ACTUATOR_SPEED;
        else if (panel_angle_deg < STOW_POSITION_DEGREES)
            desired_speed =  MAX_ACTUATOR_SPEED;
        else
            desired_speed = 0;

        log_tracker_state("HIGH WIND STOW OVERRIDE", desired_speed);
    }

    // 3. SENSOR INTEGRITY SAFETY
    else if (abs(ldr_east_lumens - ldr_west_lumens) > SENSOR_DISAGREE_LUMENS) {
        desired_speed = 0;
        log_tracker_state("LDR SENSOR DISAGREEMENT", desired_speed);
    }

    // 4. STANDARD OPERATIONAL LOGIC
    else if (system_enabled) {

        // CRV-dependent branch
        if (tracking_mode == 0) { // LDR-based
            int diff = ldr_east_lumens - ldr_west_lumens;
            if (abs(diff) > LDR_THRESHOLD)
                desired_speed = (diff > 0) ? 40 : -40;
            else
                desired_speed = 0;

            log_tracker_state("LDR TRACKING MODE", desired_speed);
        }
        else { // Predictive tracking
            desired_speed = 10;
            log_tracker_state("PREDICTIVE TRACKING MODE", desired_speed);
        }
    }

    // 5. SYSTEM DISABLED
    else {
        desired_speed = 0;
        log_tracker_state("SYSTEM DISABLED", desired_speed);
    }

    // 6. Apply slew rate and bounds
    desired_speed = apply_slew_limit(desired_speed, last_actuator_speed);
    desired_speed = clamp(desired_speed, -MAX_ACTUATOR_SPEED, MAX_ACTUATOR_SPEED);

    return desired_speed;
}

// --- Main ---
int main() {
    srand(time(NULL));
    int actuator_speed = 0;

    printf("--- Solar Panel Tracker Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_environmental_sensors();
        actuator_speed = step(actuator_speed);

    }

    return 0;
}

