#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* 
 * SAFETY INVARIANTS:
 * 1. Wind Stability: (alt > 20 && wind > 15) => (abs(pitch) <= 15)
 * 2. Power Logic: (battery < 15 && alt > 0) => (vertical_velocity <= 0)
 * 3. Temporal Payload: If weight changed > 0.5 in last 5 steps, 
 *    throttle[t] must be <= throttle[t-1].
 */

#define MAX_FLIGHT_STEPS 600
#define WINDOW_SIZE 5
#define CRITICAL_BATTERY 15.0f
#define STABLE_WEIGHT_THRESHOLD 0.5f

typedef struct {
    float altitude;
    float vertical_velocity;
    float pitch_angle;
    float battery_percent;
    float throttle_input;
    float payload_weight;
    float wind_speed;
    
    // History for temporal checks
    float weight_history[WINDOW_SIZE];
    float throttle_history[WINDOW_SIZE];
    int history_idx;
} DroneState;

typedef struct {
    int gps_satellites;
    int signal_latency_ms;
    float rain_intensity;
    int camera_tilt_pwm;
    unsigned int pilot_user_id;
    int flight_log_index;
    float signal_quality_score;
    bool encryption_ready;
} MissionData;

typedef struct {
    bool safety_halt;
    int violation_log_count;
    bool emergency_landing_active;
} SafetySystem;

// --- Function Prototypes ---
void init_drone(DroneState* d, MissionData* m, SafetySystem* s);
void simulate_flight_physics(DroneState* d);
void flight_controller(DroneState* d, SafetySystem* s);
void update_mission_metadata(MissionData* m, DroneState* d);
void run_comms_diagnostics(MissionData* m);
bool check_flight_safety(DroneState* d, SafetySystem* s);
void log_flight_history(DroneState* d);
float calculate_variance_irrelevant(MissionData* m);

int main() {
    srand((unsigned)time(NULL));

    DroneState* drone = (DroneState*)malloc(sizeof(DroneState));
    MissionData* mission = (MissionData*)malloc(sizeof(MissionData));
    SafetySystem* safety = (SafetySystem*)malloc(sizeof(SafetySystem));

    if (!drone || !mission || !safety) return 1;

    init_drone(drone, mission, safety);

    printf("--- Drone Flight Simulation Started ---\n");
    printf("Safety constraints: Wind/Pitch coupling, Battery/Descent, and Payload/Throttle stability.\n\n");

    for (int step = 0; step < MAX_FLIGHT_STEPS && !safety->safety_halt; step++) {
        
        // 1. Simulate external environment
        drone->wind_speed = 10.0f + (float)(rand() % 15);
        mission->rain_intensity = (float)(rand() % 100) / 10.0f;

        // 2. Physics Engine
        simulate_flight_physics(drone);

        // 3. Update History Buffers
        log_flight_history(drone);

        // 4. Autonomous Controller
        flight_controller(drone, safety);

        // 5. Check Safety Condition (The complex logic)
        if (!check_flight_safety(drone, safety)) {
            safety->violation_log_count++;
        }

        // 6. Non-Relevant Mission Tasks (Non-CRVs)
        update_mission_metadata(mission, drone);
        run_comms_diagnostics(mission);

        // Output Telemetry
        if (step % 40 == 0) {
            printf("[Step %d] Alt:%.1fm | Bat:%.1f%% | Pitch:%.1f | Wind:%.1f | GPS:%d | Lat:%dms\n",
                   step, drone->altitude, drone->battery_percent, drone->pitch_angle, 
                   drone->wind_speed, mission->gps_satellites, mission->signal_latency_ms);
        }

        usleep(3000); 
    }

    printf("\nFlight Simulation Finished.\n");

    free(drone); free(mission); free(safety);
    return 0;
}

void init_drone(DroneState* d, MissionData* m, SafetySystem* s) {
    d->altitude = 0.0f;
    d->vertical_velocity = 0.0f;
    d->pitch_angle = 0.0f;
    d->battery_percent = 100.0f;
    d->throttle_input = 0.0f;
    d->payload_weight = 2.5f;
    d->wind_speed = 0.0f;
    d->history_idx = 0;
    for(int i=0; i<WINDOW_SIZE; i++) {
        d->weight_history[i] = 2.5f;
        d->throttle_history[i] = 0.0f;
    }

    m->gps_satellites = 12;
    m->signal_latency_ms = 20;
    m->rain_intensity = 0.0f;
    m->camera_tilt_pwm = 1500;
    m->pilot_user_id = 88291;
    m->flight_log_index = 0;
    m->signal_quality_score = 1.0f;
    m->encryption_ready = true;

    s->safety_halt = false;
    s->violation_log_count = 0;
    s->emergency_landing_active = false;
}

void log_flight_history(DroneState* d) {
    // Simple shifting window
    for (int i = 0; i < WINDOW_SIZE - 1; i++) {
        d->weight_history[i] = d->weight_history[i+1];
        d->throttle_history[i] = d->throttle_history[i+1];
    }
    d->weight_history[WINDOW_SIZE-1] = d->payload_weight;
    d->throttle_history[WINDOW_SIZE-1] = d->throttle_input;
}

void simulate_flight_physics(DroneState* d) {
    // Battery Drain
    d->battery_percent -= 0.05f + (d->throttle_input * 0.001f);
    
    // Vertical Dynamics
    float lift = (d->throttle_input * 0.5f) / (d->payload_weight + 1.0f);
    float gravity = 9.8f * 0.01f;
    d->vertical_velocity += (lift - gravity);
    d->altitude += d->vertical_velocity;

    if (d->altitude < 0) {
        d->altitude = 0;
        d->vertical_velocity = 0;
    }

    // Shifting Payload Simulation (Occasional)
    if (rand() % 100 < 5) {
        d->payload_weight += ((float)(rand() % 10) - 5.0f) * 0.2f;
    }

    // Pitch fluctuates with wind
    d->pitch_angle = (d->wind_speed * 0.5f) + ((float)(rand() % 10) - 5.0f);
}

void flight_controller(DroneState* d, SafetySystem* s) {
    if (d->battery_percent < CRITICAL_BATTERY) {
        s->emergency_landing_active = true;
    }

    if (s->emergency_landing_active) {
        d->throttle_input = 10.0f; // Lower throttle to descend
    } else {
        if (d->altitude < 30.0f) d->throttle_input = 45.0f; // Climb
        else d->throttle_input = 20.0f; // Hover
    }
}

bool check_flight_safety(DroneState* d, SafetySystem* s) {
    bool safe = true;

    // --- Invariant 1: High Alt / High Wind Pitch Stability ---
    if (d->altitude > 20.0f && d->wind_speed > 15.0f) {
        if (d->pitch_angle > 15.0f || d->pitch_angle < -15.0f) {
            printf("[SAFETY] Aero-Stall Risk: Alt=%.1f Wind=%.1f Pitch=%.1f\n", 
                   d->altitude, d->wind_speed, d->pitch_angle);
            safe = false;
        }
    }

    // --- Invariant 2: Critical Power Descent ---
    if (d->battery_percent < CRITICAL_BATTERY && d->altitude > 0.1f) {
        if (d->vertical_velocity > 0.01f) {
            printf("[SAFETY] Power Violation: Climbing on low battery! Vel=%.2f\n", d->vertical_velocity);
            safe = false;
        }
    }

    // --- Invariant 3: Temporal Payload Stability ---
    // Check if weight varied > 0.5 in the window
    float min_w = 1000.0f, max_w = -1000.0f;
    for(int i=0; i<WINDOW_SIZE; i++) {
        if (d->weight_history[i] < min_w) min_w = d->weight_history[i];
        if (d->weight_history[i] > max_w) max_w = d->weight_history[i];
    }

    if ((max_w - min_w) > STABLE_WEIGHT_THRESHOLD) {
        // If unstable, throttle must be non-increasing in the window
        for (int i = 0; i < WINDOW_SIZE - 1; i++) {
            if (d->throttle_history[i+1] > d->throttle_history[i]) {
                printf("[SAFETY] Payload Instability: Throttle increased during shift!\n");
                safe = false;
                break;
            }
        }
    }

    if (d->battery_percent <= 0) s->safety_halt = true;

    return safe;
}

// --- NON-CONDITIONALLY RELEVANT FUNCTIONS (The Non-CRV Logic) ---

void update_mission_metadata(MissionData* m, DroneState* d) {
    m->flight_log_index++;
    
    // GPS fluctuation based on altitude (Complex but irrelevant to safety)
    if (d->altitude > 50.0f) {
        m->gps_satellites = 10 + (rand() % 4);
    } else {
        m->gps_satellites = 6 + (rand() % 6);
    }

    // Signal score based on distance (simulated as altitude here)
    m->signal_quality_score = 1.0f - (d->altitude * 0.005f);
    if (m->signal_quality_score < 0.1f) m->signal_quality_score = 0.1f;
}

void run_comms_diagnostics(MissionData* m) {
    // High complexity bitwise and arithmetic on non-safety variables
    unsigned int check = m->pilot_user_id;
    for(int i=0; i<5; i++) {
        check = ((check ^ 0xF0F0F0F0) << 2) | (check >> 30);
        check += m->flight_log_index;
    }
    
    if (check % 2 == 0) m->encryption_ready = true;
    else m->encryption_ready = false;

    // Latency simulation based on rain
    m->signal_latency_ms = 20 + (int)(m->rain_intensity * 5.5f);
}

float calculate_variance_irrelevant(MissionData* m) {
    // Heavy math on non-CRVs
    float arr[10];
    float sum = 0, mean, var = 0;
    for(int i=0; i<10; i++) {
        arr[i] = (float)(m->gps_satellites + i) * m->signal_quality_score;
        sum += arr[i];
    }
    mean = sum / 10.0f;
    for(int i=0; i<10; i++) {
        var += pow(arr[i] - mean, 2);
    }
    return var;
}

void camera_gimbal_control(MissionData* m, DroneState* d) {
    // Camera moves to compensate for pitch, but camera_tilt_pwm is not in safety logic
    int base_pwm = 1500;
    m->camera_tilt_pwm = base_pwm + (int)(d->pitch_angle * 10.0f);
    
    if (m->camera_tilt_pwm > 2000) m->camera_tilt_pwm = 2000;
    if (m->camera_tilt_pwm < 1000) m->camera_tilt_pwm = 1000;
}

void log_mission_status(MissionData* m) {
    // Dummy loop to increase code depth
    for(int i=0; i<20; i++) {
        if (m->signal_latency_ms > 100) {
            m->flight_log_index += 0; // Do nothing
        }
    }
}
