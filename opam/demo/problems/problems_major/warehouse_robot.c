#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* 
 * SAFETY INVARIANTS:
 * 1. Proximity: (dist_to_obstacle < 5.0) => (speed < 2.0)
 * 2. Stability: (load > 500 && steering != 0) => (torque <= 40.0)
 * 3. Brake Check: If speed[t] < speed[t-1] < speed[t-2] < speed[t-3], 
 *    then brake_pressure MUST be > 0.
 */

#define SIM_STEPS 500
#define OBSTACLE_THRESHOLD 5.0f
#define SPEED_LIMIT_NEAR_OBSTACLE 2.0f
#define CRITICAL_LOAD 500.0f
#define MAX_STABILITY_TORQUE 40.0f
#define SPEED_HISTORY_SIZE 4

typedef struct {
    float x_pos;
    float y_pos;
    float current_speed;
    float motor_torque;
    float steering_angle;
    float brake_pressure;
    float carried_load_kg;
    float dist_to_obstacle;
    float speed_history[SPEED_HISTORY_SIZE];
} RobotState;

typedef struct {
    int current_package_id;
    int customer_priority; // 0 to 5
    float wifi_signal_dbm;
    float odometer_km;
    int cooling_fan_rpm;
    unsigned long system_ticks;
    float ambient_humidity;
} RobotAnalytics;

typedef struct {
    bool emergency_stop;
    int violation_count;
    bool motion_inhibited;
} SafetyMonitor;

// --- Prototypes ---
void init_robot(RobotState* r, RobotAnalytics* a, SafetyMonitor* s);
void update_physics(RobotState* r);
void sensor_suite(RobotState* r, RobotAnalytics* a);
void navigation_logic(RobotState* r);
void process_analytics(RobotAnalytics* a, RobotState* r);
bool check_robot_safety(RobotState* r, SafetyMonitor* s);
void update_speed_history(RobotState* r, float new_speed);
void maintenance_diagnostics(RobotAnalytics* a);
void inventory_database_update(RobotAnalytics* a);

int main() {
    srand((unsigned)time(NULL));

    RobotState* robot = (RobotState*)malloc(sizeof(RobotState));
    RobotAnalytics* data = (RobotAnalytics*)malloc(sizeof(RobotAnalytics));
    SafetyMonitor* safety = (SafetyMonitor*)malloc(sizeof(SafetyMonitor));

    if (!robot || !data || !safety) return 1;

    init_robot(robot, data, safety);

    printf("--- Warehouse AGV Simulation Started ---\n");
    printf("Safety Logic: Proximity, Load-Stability, and Temporal Brake Consistency.\n\n");

    for (int i = 0; i < SIM_STEPS && !safety->emergency_stop; i++) {
        
        // 1. Environmental Sensors
        sensor_suite(robot, data);

        // 2. Navigation & Control
        navigation_logic(robot);

        // 3. Physics Engine
        update_physics(robot);

        // 4. Safety Property Check
        if (!check_robot_safety(robot, safety)) {
            safety->violation_count++;
        }

        // 5. Non-Relevant Analytics (The Non-CRV Logic)
        process_analytics(data, robot);
        maintenance_diagnostics(data);
        inventory_database_update(data);

        // Telemetry
        if (i % 50 == 0) {
            printf("[Step %d] Pos:(%.1f, %.1f) | Spd:%.2f | Load:%.1f | Dist:%.1f | WiFi:%.1f\n",
                   i, robot->x_pos, robot->y_pos, robot->current_speed, 
                   robot->carried_load_kg, robot->dist_to_obstacle, data->wifi_signal_dbm);
        }

        usleep(5000); 
    }

    printf("\nSimulation Complete.\n");

    free(robot); free(data); free(safety);
    return 0;
}

void init_robot(RobotState* r, RobotAnalytics* a, SafetyMonitor* s) {
    r->x_pos = 0.0f; r->y_pos = 0.0f;
    r->current_speed = 0.0f;
    r->motor_torque = 0.0f;
    r->steering_angle = 0.0f;
    r->brake_pressure = 0.0f;
    r->carried_load_kg = 600.0f; // Start with heavy load
    r->dist_to_obstacle = 20.0f;
    for(int i=0; i<SPEED_HISTORY_SIZE; i++) r->speed_history[i] = 0.0f;

    a->current_package_id = 10001;
    a->customer_priority = 3;
    a->wifi_signal_dbm = -50.0f;
    a->odometer_km = 0.0f;
    a->cooling_fan_rpm = 2000;
    a->system_ticks = 0;
    a->ambient_humidity = 45.0f;

    s->emergency_stop = false;
    s->violation_count = 0;
    s->motion_inhibited = false;
}

void update_speed_history(RobotState* r, float new_speed) {
    for (int i = 0; i < SPEED_HISTORY_SIZE - 1; i++) {
        r->speed_history[i] = r->speed_history[i + 1];
    }
    r->speed_history[SPEED_HISTORY_SIZE - 1] = new_speed;
}

void sensor_suite(RobotState* r, RobotAnalytics* a) {
    // Obstacle logic: Randomly simulate approaching a wall
    static int wall_approach = 0;
    if (wall_approach < 50) {
        r->dist_to_obstacle -= 0.3f;
    } else {
        r->dist_to_obstacle += 0.3f;
    }
    if (r->dist_to_obstacle < 1.0f) wall_approach = 100;
    if (r->dist_to_obstacle > 20.0f) wall_approach = 0;

    // Environmental noise (Non-CRVs)
    a->wifi_signal_dbm = -40.0f - (float)(rand() % 30);
    a->ambient_humidity += (rand() % 3 - 1) * 0.1f;
}

void navigation_logic(RobotState* r) {
    // Basic AI: Slow down if near obstacle
    if (r->dist_to_obstacle < 6.0f) {
        r->brake_pressure = 20.0f;
        r->motor_torque = 5.0f;
    } else {
        r->brake_pressure = 0.0f;
        r->motor_torque = 50.0f;
    }

    // Occasional turn
    static int turn_timer = 0;
    if (turn_timer++ % 100 == 0) r->steering_angle = 15.0f;
    else if (turn_timer % 100 == 20) r->steering_angle = 0.0f;
}

void update_physics(RobotState* r) {
    float prev_speed = r->current_speed;

    // Acceleration = (Torque - Brakes - Friction)
    float accel = (r->motor_torque * 0.1f) - (r->brake_pressure * 0.5f) - 0.05f;
    r->current_speed += accel;

    if (r->current_speed < 0) r->current_speed = 0;
    if (r->current_speed > 5.0f) r->current_speed = 5.0f;

    // Displacement
    r->x_pos += r->current_speed * cos(r->steering_angle * 0.0174f);
    r->y_pos += r->current_speed * sin(r->steering_angle * 0.0174f);

    update_speed_history(r, r->current_speed);
}

bool check_robot_safety(RobotState* r, SafetyMonitor* s) {
    bool is_safe = true;

    // --- Invariant 1: Proximity Limit ---
    if (r->dist_to_obstacle < OBSTACLE_THRESHOLD) {
        if (r->current_speed >= SPEED_LIMIT_NEAR_OBSTACLE) {
            printf("[SAFETY] Speed violation near obstacle: Spd=%.2f Dist=%.2f\n", 
                    r->current_speed, r->dist_to_obstacle);
            is_safe = false;
        }
    }

    // --- Invariant 2: Load Stability ---
    if (r->carried_load_kg > CRITICAL_LOAD && r->steering_angle != 0) {
        if (r->motor_torque > MAX_STABILITY_TORQUE) {
            printf("[SAFETY] Tipping hazard! High torque (%.2f) while turning with load (%.2f)\n", 
                    r->motor_torque, r->carried_load_kg);
            is_safe = false;
        }
    }

    // --- Invariant 3: Temporal Brake Check (The Loop) ---
    // Rule: If speed is strictly decreasing across the window, brake must be active.
    bool strictly_decreasing = true;
    for (int i = 0; i < SPEED_HISTORY_SIZE - 1; i++) {
        if (r->speed_history[i+1] >= r->speed_history[i]) {
            strictly_decreasing = false;
            break;
        }
    }

    if (strictly_decreasing && r->current_speed > 0.1f) {
        if (r->brake_pressure <= 0.0f) {
            printf("[SAFETY] Deceleration detected without braking! System failure.\n");
            is_safe = false;
        }
    }

    return is_safe;
}

// --- NON-CONDITIONALLY RELEVANT FUNCTIONS (Over 200 lines requirement) ---

void process_analytics(RobotAnalytics* a, RobotState* r) {
    a->system_ticks++;
    a->odometer_km += (r->current_speed * 0.001f);

    // Fan speed logic based on load (Irrelevant to safety condition)
    if (r->carried_load_kg > 200.0f) {
        a->cooling_fan_rpm = 3500;
    } else {
        a->cooling_fan_rpm = 1500;
    }
}

void maintenance_diagnostics(RobotAnalytics* a) {
    // Complex bitwise logic on non-relevant ID
    unsigned int check = (unsigned int)a->current_package_id;
    check = ~check;
    check = check ^ 0xDEADBEEF;
    
    // Simulate a diagnostic interval
    if (a->system_ticks % 100 == 0) {
        a->customer_priority = (rand() % 5);
    }

    // This data transformation is dense but never touches safety variables
    float humidity_factor = a->ambient_humidity / 100.0f;
    a->wifi_signal_dbm -= humidity_factor;
}

void inventory_database_update(RobotAnalytics* a) {
    // Simulated database logic
    static int db_sync_status = 0;
    db_sync_status = (a->current_package_id * 17) % 100;

    if (db_sync_status > 50) {
        // Mock processing steps
        for(int i=0; i<10; i++) {
            a->system_ticks += i;
            a->system_ticks -= i;
        }
    }
}

void auxiliary_reporting_service(RobotAnalytics* a, RobotState* r) {
    // Generate complex logs that reference many variables but affect none
    if (a->wifi_signal_dbm < -80.0f) {
        // Low signal - doesn't affect robot braking or speed
        int log_code = (int)r->x_pos ^ (int)r->y_pos;
        a->current_package_id = 20000 + (log_code % 1000);
    }
}
