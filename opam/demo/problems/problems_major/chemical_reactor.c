#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* 
 * TARGET SAFETY PROPERTY (The Invariants):
 * 1. (Internal Pressure > 150 PSI) => (Emergency Vent == OPEN)
 * 2. (Concentration > 80%) => (Instant Temp Delta < 8.0K)
 * 3. (Pump at 100% for >= 5 cycles) => Temperature sequence [T0...T4] 
 *    must NOT be strictly increasing.
 */

#define MAX_CYCLES 400
#define CRITICAL_PRESSURE 150.0f
#define HIGH_CONCENTRATION 80.0f
#define MAX_TEMP_DELTA 8.0f
#define WINDOW_SIZE 5
#define COOLING_THRESHOLD 340.0f

typedef struct {
    float internal_temp;
    float prev_temp;
    float internal_pressure;
    float reactant_conc;
    int emergency_vent;    // 0 = Closed, 1 = Open
    int cooling_pump_pwr;  // 0 to 100
} ReactorState;

typedef struct {
    int operator_id;
    int batch_id;
    float yield_index;
    float ambient_humidity;
    int light_intensity;
    float sensor_noise_floor;
    unsigned int batch_signature;
    char status_code[8];
} MonitoringData;

typedef struct {
    float temp_window[WINDOW_SIZE]; // Captures values for the 5-cycle trend
    int pump_max_duration;
    bool system_halted;
    int total_violations;
} SafetyController;

// --- Function Prototypes ---
void init_systems(ReactorState* r, MonitoringData* m, SafetyController* s);
void simulate_reaction_physics(ReactorState* r);
void run_control_loops(ReactorState* r);
void process_logging_and_quality(MonitoringData* m, ReactorState* r);
void calculate_batch_signature(MonitoringData* m);
bool monitor_safety_invariants(ReactorState* r, SafetyController* s);
void run_auxiliary_diagnostics(MonitoringData* m, ReactorState* r);
void update_operator_display(MonitoringData* m, ReactorState* r);
void generate_environmental_noise(MonitoringData* m);

int main() {
    srand((unsigned)time(NULL));

    ReactorState* reactor = (ReactorState*)malloc(sizeof(ReactorState));
    MonitoringData* logs = (MonitoringData*)malloc(sizeof(MonitoringData));
    SafetyController* safety = (SafetyController*)malloc(sizeof(SafetyController));

    if (!reactor || !logs || !safety) {
        fprintf(stderr, "Memory Allocation Failed\n");
        return 1;
    }

    init_systems(reactor, logs, safety);

    printf("--- Chemical Reactor Safety Benchmark Started ---\n");
    printf("Safety Condition: If Pump=100%%, Temp trend cannot be strictly increasing.\n\n");

    int cycle = 0;
    while (cycle < MAX_CYCLES && !safety->system_halted) {
        
        // 1. Update Environmental Non-CRVs (Noise)
        generate_environmental_noise(logs);

        // 2. Core Physics (State transitions)
        simulate_reaction_physics(reactor);

        // 3. Controller logic (Decides Vent and Pump based on State)
        run_control_loops(reactor);

        // 4. Auxiliary logic (Processing non-relevant variables)
        process_logging_and_quality(logs, reactor);
        calculate_batch_signature(logs);
        run_auxiliary_diagnostics(logs, reactor);
        update_operator_display(logs, reactor);

        // 5. Safety Invariant Check (The property monitor)
        if (!monitor_safety_invariants(reactor, safety)) {
            safety->total_violations++;
        }

        // Periodic Telemetry Output
        if (cycle % 25 == 0) {
            printf("[Cyc:%3d] T:%6.2fK | P:%6.2fPSI | C:%5.1f%% | Vent:%d | Pump:%d | Sig:%X\n",
                   cycle, reactor->internal_temp, reactor->internal_pressure, 
                   reactor->reactant_conc, reactor->emergency_vent,
                   reactor->cooling_pump_pwr, logs->batch_signature);
        }

        cycle++;
        usleep(5000); // 5ms simulated step
    }

    printf("\nSimulation Ended at cycle %d.\n", cycle);

    free(reactor);
    free(logs);
    free(safety);
    return 0;
}

void init_systems(ReactorState* r, MonitoringData* m, SafetyController* s) {
    r->internal_temp = 300.0f;
    r->prev_temp = 300.0f;
    r->internal_pressure = 14.7f;
    r->reactant_conc = 100.0f;
    r->emergency_vent = 0;
    r->cooling_pump_pwr = 0;

    m->operator_id = rand() % 500 + 1000;
    m->batch_id = 8821;
    m->yield_index = 0.0f;
    m->ambient_humidity = 40.0f;
    m->light_intensity = 450;
    m->sensor_noise_floor = 0.02f;
    m->batch_signature = 0xAA00BB11;

    s->pump_max_duration = 0;
    s->system_halted = false;
    s->total_violations = 0;
    
    // Initialize the temporal window with the starting temperature
    for(int i = 0; i < WINDOW_SIZE; i++) {
        s->temp_window[i] = 300.0f;
    }
}

void simulate_reaction_physics(ReactorState* r) {
    r->prev_temp = r->internal_temp;

    // Simulation of exothermic reaction behavior
    if (r->reactant_conc > 0.05f) {
        // Temperature influences reaction rate (Arrhenius-like)
        float rate = (r->internal_temp / 280.0f) * 0.45f;
        r->reactant_conc -= rate;
        
        // Exothermic heat release
        r->internal_temp += (r->reactant_conc * 0.065f);
    }

    // Pressure calculation (Gas expansion)
    r->internal_pressure = (r->internal_temp * 0.048f) * (1.1f + (100.0f - r->reactant_conc) * 0.008f);

    // Active cooling influence (Pump)
    r->internal_temp -= (r->cooling_pump_pwr * 0.14f);
    
    // Venting influence (Pressure relief)
    if (r->emergency_vent == 1) {
        r->internal_pressure -= 12.5f;
        r->internal_temp -= 1.8f;
    }

    // Physical sanity bounds
    if (r->internal_pressure < 14.7f) r->internal_pressure = 14.7f;
    if (r->internal_temp < 273.15f) r->internal_temp = 273.15f;
}

void run_control_loops(ReactorState* r) {
    // Determine cooling intensity based on current state
    if (r->internal_temp > COOLING_THRESHOLD) {
        r->cooling_pump_pwr = 100;
    } else if (r->internal_temp > 315.0f) {
        r->cooling_pump_pwr = 40;
    } else {
        r->cooling_pump_pwr = 5;
    }

    // Automated Emergency Venting logic
    if (r->internal_pressure > 148.0f) {
        r->emergency_vent = 1;
    } else {
        r->emergency_vent = 0;
    }
}

void process_logging_and_quality(MonitoringData* m, ReactorState* r) {
    /* Variables here are non-CRVs for the safety condition */
    float efficiency_factor = (float)m->operator_id / 2000.0f;
    m->yield_index = (100.0f - r->reactant_conc) * 0.92f + efficiency_factor;
    
    if (m->ambient_humidity > 60.0f) {
        m->sensor_noise_floor = 0.08f;
    } else {
        m->sensor_noise_floor = 0.01f;
    }
}

void calculate_batch_signature(MonitoringData* m) {
    // Computational logic on non-relevant data (Opaque to safety)
    unsigned int sig = (unsigned int)m->batch_id;
    sig = ((sig << 8) | (sig >> 24)) ^ 0xFFEEFFEE;
    sig += (unsigned int)(m->yield_index * 100);
    m->batch_signature = sig;
}

void generate_environmental_noise(MonitoringData* m) {
    m->ambient_humidity += ((rand() % 10) - 5) * 0.1f;
    if (m->ambient_humidity < 10.0f) m->ambient_humidity = 10.0f;
}

bool monitor_safety_invariants(ReactorState* r, SafetyController* s) {
    bool safe = true;

    // UPDATE HISTORY: Shift window and insert current temp
    for (int i = 0; i < WINDOW_SIZE - 1; i++) {
        s->temp_window[i] = s->temp_window[i + 1];
    }
    s->temp_window[WINDOW_SIZE - 1] = r->internal_temp;

    // --- Invariant 1: Pressure/Vent Check (Immediate) ---
    if (r->internal_pressure > CRITICAL_PRESSURE) {
        if (r->emergency_vent != 1) {
            printf("[SAFETY FAILURE] High Pressure (%.2f) with Vent CLOSED!\n", r->internal_pressure);
            safe = false;
        }
    }

    // --- Invariant 2: Exothermic Ramp Check (Immediate) ---
    if (r->reactant_conc > HIGH_CONCENTRATION) {
        float instant_delta = r->internal_temp - r->prev_temp;
        if (instant_delta > MAX_TEMP_DELTA) {
            printf("[SAFETY FAILURE] Rapid Temp Increase (%.2f) at high concentration!\n", instant_delta);
            safe = false;
        }
    }

    // --- Invariant 3: Temporal Cooling Efficiency (Window Trend) ---
    if (r->cooling_pump_pwr == 100) {
        s->pump_max_duration++;
    } else {
        s->pump_max_duration = 0;
    }

    if (s->pump_max_duration >= WINDOW_SIZE) {
        bool strictly_increasing = true;
        for (int i = 0; i < WINDOW_SIZE - 1; i++) {
            // If even one step is stable or decreasing, it's not strictly increasing
            if (s->temp_window[i + 1] <= s->temp_window[i]) {
                strictly_increasing = false;
                break;
            }
        }

        if (strictly_increasing) {
            printf("[!] Safety Fail: Thermal Runaway! Temp strictly increasing: ");
            printf("[%.1f < %.1f < %.1f < %.1f < %.1f]\n",
                   s->temp_window[0], s->temp_window[1], s->temp_window[2],
                   s->temp_window[3], s->temp_window[4]);
            safe = false;
        }
    }

    // Global catastrophic threshold
    if (r->internal_temp > 550.0f || r->internal_pressure > 250.0f) {
        printf("[FATAL] Physical limits exceeded. System Halted.\n");
        s->system_halted = true;
        safe = false;
    }

    return safe;
}

void run_auxiliary_diagnostics(MonitoringData* m, ReactorState* r) {
    // Heavy calculations that do not propagate to safety-relevant variables
    float diag_val = (r->reactant_conc * m->ambient_humidity) / 100.0f;
    int code_seed = (int)diag_val % 26;
    
    for (int i = 0; i < 4; i++) {
        m->status_code[i] = 'A' + ((code_seed + i) % 26);
    }
    m->status_code[4] = '\0';
}

void update_operator_display(MonitoringData* m, ReactorState* r) {
    // UI logic (Irrelevant to core safety)
    static int blink_counter = 0;
    blink_counter++;

    if (r->internal_temp > 400.0f) {
        m->light_intensity = (blink_counter % 2 == 0) ? 999 : 0;
    }

    // Opaque arithmetic loop for complexity
    double aggregator = 0.0;
    for(int i = 0; i < 40; i++) {
        aggregator += cos((double)m->light_intensity + i);
    }
    m->sensor_noise_floor += (float)(aggregator * 0.00001);
}
