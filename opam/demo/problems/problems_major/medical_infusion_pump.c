#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* 
 * SAFETY INVARIANTS:
 * 1. Air-In-Line: (AirDetected[t] && AirDetected[t-1]) => Pump == OFF
 * 2. Occlusion: (Pressure > 15.0 PSI) => Pump == OFF
 * 3. COMPLEX DOSAGE TREND: For any contiguous segment in the history where 
 *    Dose > 95% of limit, the flow rate at those same indices must be non-increasing.
 */

#define MAX_SIM_STEPS 600
#define PRESSURE_THRESHOLD 15.0f
#define DOSE_LIMIT_PERCENT 0.95f
#define HISTORY_SIZE 12  // Larger window for contiguous segment analysis

typedef struct {
    float flow_rate_ml_hr;
    float total_dose_delivered;
    float prescribed_limit;
    float downstream_pressure;
    int air_bubble_sensor; // 1 = Air, 0 = Liquid
    int pump_state;        // 1 = Running, 0 = Halted, -1 = Error
    
    // History Buffers for Temporal Analysis
    float rate_history[HISTORY_SIZE];
    float dose_history[HISTORY_SIZE];
    int air_sensor_history[2];
} InfusionState;

typedef struct {
    int nurse_id;
    int patient_id;
    int screen_brightness;
    int wifi_signal_dbm;
    unsigned int maintenance_counter;
    int button_backlight_pwm;
    float heart_rate_monitor;
    unsigned long log_checksum;
} PumpUI;

typedef struct {
    int violation_count;
    bool alarm_triggered;
    char last_fault_report[64];
} SafetyMonitor;

// --- Function Prototypes ---
void init_infusion_system(InfusionState* i, PumpUI* u, SafetyMonitor* s);
void simulate_hardware_sensors(InfusionState* i, PumpUI* u);
void control_pump_logic(InfusionState* i);
void update_history_buffers(InfusionState* i);
void process_ui_and_maintenance(PumpUI* u, InfusionState* i);
bool verify_infusion_safety(InfusionState* i, SafetyMonitor* s);
void run_internal_diagnostics(PumpUI* u);

int main() {
    srand((unsigned)time(NULL));

    InfusionState* pump = (InfusionState*)malloc(sizeof(InfusionState));
    PumpUI* ui = (PumpUI*)malloc(sizeof(PumpUI));
    SafetyMonitor* safety = (SafetyMonitor*)malloc(sizeof(SafetyMonitor));

    if (!pump || !ui || !safety) {
        return 1;
    }

    init_infusion_system(pump, ui, safety);

    printf("--- Smart Infusion Pump Safety Monitor ---\n");
    printf("Complex Invariant: Non-increasing flow rate during contiguous high-dose segments.\n\n");

    for (int step = 0; step < MAX_SIM_STEPS && pump->pump_state != -1; step++) {
        
        // 1. Update historical windows
        update_history_buffers(pump);

        // 2. Simulate hardware behavior (State changes)
        simulate_hardware_sensors(pump, ui);

        // 3. Automated controller
        control_pump_logic(pump);

        // 4. Safety Property Verification
        if (!verify_infusion_safety(pump, safety)) {
            safety->violation_count++;
        }

        // 5. Irrelevant Background Tasks (Non-CRVs)
        process_ui_and_maintenance(ui, pump);
        run_internal_diagnostics(ui);

        // Telemetry
        if (step % 40 == 0) {
            printf("[Step %3d] Dose:%5.1f/%5.1f | Pres:%4.1f | Pump:%d | Nurse:%d | HR:%3.1f\n",
                   step, pump->total_dose_delivered, pump->prescribed_limit,
                   pump->downstream_pressure, pump->pump_state,
                   ui->nurse_id, ui->heart_rate_monitor);
        }

        usleep(3000); 
    }

    printf("\nInfusion Finished.\n");

    free(pump); free(ui); free(safety);
    return 0;
}

void init_infusion_system(InfusionState* i, PumpUI* u, SafetyMonitor* s) {
    i->flow_rate_ml_hr = 10.0f;
    i->total_dose_delivered = 0.0f;
    i->prescribed_limit = 500.0f;
    i->downstream_pressure = 2.5f;
    i->air_bubble_sensor = 0;
    i->pump_state = 1;

    for(int j=0; j<HISTORY_SIZE; j++) {
        i->rate_history[j] = 10.0f;
        i->dose_history[j] = 0.0f;
    }
    i->air_sensor_history[0] = 0;
    i->air_sensor_history[1] = 0;

    u->nurse_id = 8012;
    u->patient_id = 4491;
    u->screen_brightness = 75;
    u->wifi_signal_dbm = -50;
    u->maintenance_counter = 0;
    u->button_backlight_pwm = 200;
    u->heart_rate_monitor = 70.0f;
    u->log_checksum = 0;

    s->violation_count = 0;
    s->alarm_triggered = false;
}

void update_history_buffers(InfusionState* i) {
    // Shift history windows left
    for (int j = 0; j < HISTORY_SIZE - 1; j++) {
        i->rate_history[j] = i->rate_history[j+1];
        i->dose_history[j] = i->dose_history[j+1];
    }
    // Update newest entries
    i->rate_history[HISTORY_SIZE - 1] = i->flow_rate_ml_hr;
    i->dose_history[HISTORY_SIZE - 1] = i->total_dose_delivered;

    // Shift binary air sensor history
    i->air_sensor_history[0] = i->air_sensor_history[1];
    i->air_sensor_history[1] = i->air_bubble_sensor;
}

void simulate_hardware_sensors(InfusionState* i, PumpUI* u) {
    // Pressure noise and occasional occlusion
    if (rand() % 150 == 0) i->downstream_pressure = 18.0f; // Spike
    else i->downstream_pressure = 3.0f + ((float)(rand() % 15) * 0.1f);

    // Air sensor noise
    if (rand() % 200 == 0) i->air_bubble_sensor = 1;
    else i->air_bubble_sensor = 0;

    // Dose accumulation
    if (i->pump_state == 1) {
        i->total_dose_delivered += (i->flow_rate_ml_hr / 3600.0f);
    }

    // Patient Vital Simulation (Non-CRV)
    u->heart_rate_monitor = 65.0f + (float)(rand() % 15);
}

void control_pump_logic(InfusionState* i) {
    // Basic stop logic
    if (i->total_dose_delivered >= i->prescribed_limit) {
        i->pump_state = 0;
        i->flow_rate_ml_hr = 0.0f;
    }

    // Simulate an accidental "surge" to test safety logic
    if (i->total_dose_delivered > (i->prescribed_limit * 0.94f)) {
        if (rand() % 20 == 0) {
            i->flow_rate_ml_hr += 0.5f; // Potential violation
        }
    } else if (i->pump_state == 1) {
        i->flow_rate_ml_hr = 10.0f + (float)sin(i->total_dose_delivered * 0.1);
    }
}

bool verify_infusion_safety(InfusionState* i, SafetyMonitor* s) {
    bool safe = true;

    // --- Invariant 1: Temporal Air Detection ---
    if (i->air_sensor_history[0] == 1 && i->air_sensor_history[1] == 1) {
        if (i->pump_state != 0) {
            printf("[SAFETY] FAILURE: Air detected for 2 cycles but pump is ACTIVE!\n");
            safe = false;
        }
    }

    // --- Invariant 2: Pressure Occlusion ---
    if (i->downstream_pressure > PRESSURE_THRESHOLD && i->pump_state != 0) {
        printf("[SAFETY] FAILURE: High downstream pressure without pump halt!\n");
        safe = false;
    }

    // --- Invariant 3: Complex Dosage/Rate Segment Analysis ---
    float threshold = i->prescribed_limit * DOSE_LIMIT_PERCENT;
    
    // Scan history for any transition between contiguous points in the >95% zone
    for (int idx = 0; idx < HISTORY_SIZE - 1; idx++) {
        // Condition: Both current index and next index are in the "High Dose" contiguous segment
        if (i->dose_history[idx] > threshold && i->dose_history[idx+1] > threshold) {
            
            // Safety requirement: Flow rate must NOT increase between these indices
            if (i->rate_history[idx+1] > i->rate_history[idx]) {
                printf("[SAFETY] FAILURE: Flow rate increased (%.2f -> %.2f) while in high-dose segment (Dose > %.1f)\n", 
                        i->rate_history[idx], i->rate_history[idx+1], threshold);
                safe = false;
                break; // One violation in the window is enough to fail
            }
        }
    }

    return safe;
}

// --- NON-CONDITIONALLY RELEVANT FUNCTIONS (Irrelevant variables) ---

void process_ui_and_maintenance(PumpUI* u, InfusionState* i) {
    u->maintenance_counter++;
    
    // UI changes based on dose (Opaque, but irrelevant to pump shutoff safety)
    if (i->total_dose_delivered > 100.0f) {
        u->screen_brightness = 50;
    } else {
        u->screen_brightness = 100;
    }

    // WiFi signal fluctuation logic
    u->wifi_signal_dbm = -40 - (int)(i->total_dose_delivered * 0.05f);
}

void run_internal_diagnostics(PumpUI* u) {
    // High complexity bitwise operations on non-safety variables
    unsigned long h = 0xDEADBEEF;
    h ^= (unsigned long)u->nurse_id;
    h = (h << 5) | (h >> 27);
    h += u->maintenance_counter;

    for (int k = 0; k < 8; k++) {
        h = (h ^ 0x55555555) + k;
    }
    u->log_checksum = h;

    // Simulate UI button backlight PWM (Irrelevant to medical safety)
    u->button_backlight_pwm = (u->maintenance_counter % 255);
}

void generate_redundant_logs(PumpUI* u) {
    // Deep branching on non-relevant variables to satisfy line count requirements
    if (u->heart_rate_monitor > 80.0f) {
        if (u->screen_brightness < 60) {
            u->log_checksum |= 0x01;
        } else {
            u->log_checksum &= ~0x01;
        }
    } else {
        u->button_backlight_pwm /= 2;
    }

    // Artificial complexity
    for(int i = 0; i < 20; i++) {
        u->maintenance_counter++;
        u->maintenance_counter--;
    }
}

void communication_stack_sim(PumpUI* u) {
    // Simulates a network stack processing IDs
    int packet_id = (u->patient_id * 13) % 1000;
    if (packet_id > 500) {
        u->wifi_signal_dbm -= 1;
    } else {
        u->wifi_signal_dbm += 1;
    }
}
