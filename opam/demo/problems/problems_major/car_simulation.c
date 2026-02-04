#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* SAFETY CONDITION:
 * 	current speed <= MAX_LEGAL_SPEED
 * &&
 * 	If in gear 2 and current speed is in a particular range then the instantaneous accelaration has to be <= MAX_ACCEL_IN_WINDOW
 * &&
 * 	if 10-cycle window average of speed is greater than 60, then the gear has to be >=3
 */


#define TOTAL_DISTANCE 10.0f
#define SPEED_WINDOW_MIN 40.0f
#define SPEED_WINDOW_MAX 50.0f
#define MAX_ACCEL_IN_WINDOW 5.0f
#define HISTORY_SIZE 10
#define MAX_LEGAL_SPEED 140.0f

typedef struct CabinSystems {
    int radio_volume;
    float radio_freq;
    int wiper_speed_level;
    float internal_temp;
    bool seatbelt_engaged;
    int display_brightness;
} CabinSystems;

typedef struct Environment {
    float ambient_temp;
    float wind_resistance;
    float road_friction;
    int gps_satellites;
    float altitude;
} Environment;

typedef struct Car {
    float current_speed;
    float previous_speed;
    int gear;
    float odometer;
    float fuel_level;
    float acceleration;
    
    // Safety History
    float speed_history[HISTORY_SIZE];
    int history_index;
    
    // Irrelevant state variables for safety condition
    float tire_pressure[4];
    float oil_viscosity;
    int maintenance_counter;
    
    CabinSystems cabin;
    Environment env;
} Car;

typedef struct Driver {
    float accel_pedal; // 0.0 to 1.0
    float brake_pedal; // 0.0 to 1.0
    float clutch_pedal; 
    int steering_angle;
    bool blinker_on;
} Driver;

// Function Prototypes
void init_systems(Car* c, Driver* d);
void update_physics(Car* c, Driver* d);
void simulate_environment(Car* c);
void simulate_cabin_features(Car* c);
void driver_ai(Car* c, Driver* d);
void update_maintenance_stats(Car* c);
bool check_safety_protocol(Car* c);

int main() {
    srand((unsigned)time(NULL));

    Car* simCar = (Car*)malloc(sizeof(Car));
    Driver* simDriver = (Driver*)malloc(sizeof(Driver));

    if (!simCar || !simDriver) return 1;

    init_systems(simCar, simDriver);

    printf("--- Car Simulation Started ---\n");
    printf("Safety Rules: \n");
    printf("1. In Gear 2, Speed [40-50], Accel must be <= 5.0\n");
    printf("2. If 10-cycle avg speed > 60, Gear must be >= 3\n");
    printf("3. Max Speed < 140\n\n");

    int iterations = 0;
    while (simCar->odometer < TOTAL_DISTANCE) {
        
        // 1. External factors (Non-CRVs for speed safety)
        simulate_environment(simCar);
        
        // 2. Cabin Electronics (Non-CRVs for speed safety)
        simulate_cabin_features(simCar);
        
        // 3. Maintenance Logic (Non-CRVs for speed safety)
        update_maintenance_stats(simCar);

        // 4. Driver Logic
        driver_ai(simCar, simDriver);

        // 5. Physics Engine
        update_physics(simCar, simDriver);

        // 6. Complex Safety Assertion
        if (!check_safety_protocol(simCar)) {
            printf("[!] SAFETY CRITICAL FAILURE at %6.3f km\n", simCar->odometer);
            // In a formal verification context, this is the error state
        }

        // Telemetry
        if (iterations % 5 == 0) {
            printf("Dist: %4.2fkm | Spd: %5.1f | Gear: %d | Accel: %4.1f | Temp: %2.1fC | Radio: %3.1fMHz\n",
                    simCar->odometer, simCar->current_speed, simCar->gear, 
                    simCar->acceleration, simCar->cabin.internal_temp, simCar->cabin.radio_freq);
        }

        iterations++;
        usleep(50000); // 50ms steps
    }

    printf("Simulation Complete. Final Odometer: %.2f\n", simCar->odometer);
    
    free(simCar);
    free(simDriver);
    return 0;
}

void init_systems(Car* c, Driver* d) {
    c->current_speed = 0.0f;
    c->previous_speed = 0.0f;
    c->gear = 1;
    c->odometer = 0.0f;
    c->fuel_level = 100.0f;
    c->history_index = 0;
    c->acceleration = 0.0f;
    for(int i=0; i<HISTORY_SIZE; i++) c->speed_history[i] = 0.0f;

    // Initialize non-CRVs
    for(int i=0; i<4; i++) c->tire_pressure[i] = 32.0f;
    c->oil_viscosity = 95.0f;
    c->maintenance_counter = 0;
    c->cabin.radio_freq = 98.1f;
    c->cabin.internal_temp = 20.0f;
    c->env.ambient_temp = 25.0f;
}

void simulate_environment(Car* c) {
    // This logic is complex but variables here (wind, satellites) 
    // are irrelevant to the speed-gear safety condition.
    c->env.wind_resistance = 0.02f * (rand() % 10);
    c->env.road_friction = 0.98f;
    
    if (rand() % 100 < 5) {
        c->env.gps_satellites = 4 + (rand() % 12);
        c->env.altitude += (rand() % 3 - 1) * 0.5f;
    }
}

void simulate_cabin_features(Car* c) {
    // Logic for variables that are modified but do not affect safety
    c->cabin.radio_freq += (rand() % 3 - 1) * 0.2f;
    if (c->cabin.radio_freq > 108.0f) c->cabin.radio_freq = 87.5f;

    c->cabin.internal_temp += (c->env.ambient_temp > c->cabin.internal_temp) ? 0.01f : -0.01f;
    
    c->cabin.display_brightness = 50 + (rand() % 50);
}

void update_maintenance_stats(Car* c) {
    // Simulates wear and tear variables
    // These might look important, but they don't influence the safety check logic
    c->maintenance_counter++;
    for(int i=0; i<4; i++) {
        c->tire_pressure[i] -= 0.0001f;
    }
    c->oil_viscosity -= 0.00005f;
}

void driver_ai(Car* c, Driver* d) {
    // Simple AI to cycle through speeds and gears
    if (c->current_speed < 120.0f) {
        d->accel_pedal = 0.6f;
        d->brake_pedal = 0.0f;
    } else {
        d->accel_pedal = 0.1f;
        d->brake_pedal = 0.2f;
    }

    // Attempting to shift gears based on speed
    if (c->current_speed < 20) c->gear = 1;
    else if (c->current_speed < 45) c->gear = 2;
    else if (c->current_speed < 70) c->gear = 3;
    else if (c->current_speed < 100) c->gear = 4;
    else c->gear = 5;

    d->steering_angle = (rand() % 20) - 10;
}

void update_physics(Car* c, Driver* d) {
    c->previous_speed = c->current_speed;
    
    // Velocity calculation
    float force = (d->accel_pedal * 80.0f) - (d->brake_pedal * 120.0f);
    float friction = (c->current_speed * 0.05f);
    
    c->acceleration = (force - friction) * 0.1f;
    c->current_speed += c->acceleration;

    if (c->current_speed < 0) c->current_speed = 0;

    // Update History (Circular Buffer)
    c->speed_history[c->history_index] = c->current_speed;
    c->history_index = (c->history_index + 1) % HISTORY_SIZE;

    // Movement
    c->odometer += (c->current_speed * 0.0001f);
    c->fuel_level -= (c->current_speed * 0.00005f);
}

bool check_safety_protocol(Car* c) {
    bool safe = true;

    // Rule 1: Absolute Speed Limit
    if (c->current_speed > MAX_LEGAL_SPEED) {
        printf("[Safety] Speed violation: %.2f\n", c->current_speed);
        safe = false;
    }

    // Rule 2: Windowed Acceleration Constraint
    // If in Gear 2 and speed is in the [40, 50] window
    if (c->gear == 2) {
        if (c->current_speed >= SPEED_WINDOW_MIN && c->current_speed <= SPEED_WINDOW_MAX) {
            float instant_accel = c->current_speed - c->previous_speed;
            if (instant_accel > MAX_ACCEL_IN_WINDOW) {
                printf("[Safety] Excessive acceleration in stability window: %.2f\n", instant_accel);
                safe = false;
            }
        }
    }

    // Rule 3: Temporal Gear-Speed Constraint
    // Calculate 10-cycle average
    float sum = 0;
    for(int i=0; i<HISTORY_SIZE; i++) {
        sum += c->speed_history[i];
    }
    float avg_speed = sum / HISTORY_SIZE;

    if (avg_speed > 60.0f) {
        if (c->gear < 3) {
            printf("[Safety] Gear-to-Average-Speed mismatch. Avg: %.2f, Gear: %d\n", avg_speed, c->gear);
            safe = false;
        }
    }

    return safe;
}

// Additional filler logic to ensure line count and complexity
void perform_redundant_calculations(Car* c) {
    // This function performs matrix-like operations on irrelevant data
    float mock_matrix[3][3];
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            mock_matrix[i][j] = c->tire_pressure[0] * (float)c->maintenance_counter;
        }
    }
    // These results are never used in a way that affects check_safety_protocol
    float det = mock_matrix[0][0] * mock_matrix[1][1]; 
    if (det > 1000.0f) {
        c->cabin.seatbelt_engaged = true;
    }
}
