#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>

/* 
 * SAFETY INVARIANTS:
 * 1. Overcurrent: (Load > Rating) for 3 steps => Breaker must be OPEN (0).
 * 2. Short Circuit: (Breaker == 1) && (Voltage / Load < 0.5) => Fault Indicator == 1.
 * 3. Frequency: abs(Freq - 60.0) > 0.8 => Phase Correction must be 1.
 */

#define MAX_SIM_CYCLES 450
#define ZONE_COUNT 3
#define OVERLOAD_WINDOW 3
#define NOMINAL_FREQ 60.0f
#define FREQ_TOLERANCE 0.8f
#define SHORT_CIRCUIT_IMPEDANCE 0.5f

typedef struct {
    float bus_voltage;
    float active_load_amps[ZONE_COUNT];
    float breaker_rating[ZONE_COUNT];
    int breaker_status[ZONE_COUNT]; // 1 = Closed, 0 = Open
    float grid_frequency;
    int phase_correction_active;
    int fault_indicator;
    
    // Temporal data
    float load_history[ZONE_COUNT][OVERLOAD_WINDOW];
} GridState;

typedef struct {
    double total_billing_kwh;
    float customer_rate_per_kwh;
    unsigned int packet_id;
    int operator_privilege;
    float cooling_fan_runtime;
    float panel_temperature;
    unsigned long communication_hash;
    int ambient_light_lux;
} UtilityMetrics;

typedef struct {
    int safety_violations;
    bool grid_shutdown;
    char last_error_log[64];
} SafetyMonitor;

// --- Prototypes ---
void init_substation(GridState* g, UtilityMetrics* u, SafetyMonitor* s);
void simulate_power_flow(GridState* g);
void grid_controller(GridState* g);
void update_utility_billing(UtilityMetrics* u, GridState* g);
void run_network_diagnostics(UtilityMetrics* u);
bool verify_substation_integrity(GridState* g, SafetyMonitor* s);
void shift_load_history(GridState* g);
void maintenance_subroutine(UtilityMetrics* u, GridState* g);

int main() {
    srand((unsigned)time(NULL));

    GridState* grid = (GridState*)malloc(sizeof(GridState));
    UtilityMetrics* util = (UtilityMetrics*)malloc(sizeof(UtilityMetrics));
    SafetyMonitor* safety = (SafetyMonitor*)malloc(sizeof(SafetyMonitor));

    if (!grid || !util || !safety) return 1;

    init_substation(grid, util, safety);

    printf("--- Smart Grid Substation Monitoring Started ---\n");
    printf("Invariants: Sustained Overcurrent, Impedance Faults, and Phase Stability.\n\n");

    for (int cycle = 0; cycle < MAX_SIM_CYCLES && !safety->grid_shutdown; cycle++) {
        
        // 1. Shift temporal data for windowed checks
        shift_load_history(grid);

        // 2. Physics: Power Flow Simulation
        simulate_power_flow(grid);

        // 3. Automated Control Logic
        grid_controller(grid);

        // 4. Safety Property Check (The monitor)
        if (!verify_substation_integrity(grid, safety)) {
            safety->safety_violations++;
        }

        // 5. Non-Relevant Modules (Non-CRVs)
        update_utility_billing(util, grid);
        run_network_diagnostics(util);
        maintenance_subroutine(util, grid);

        // Telemetry
        if (cycle % 50 == 0) {
            printf("[Cyc %d] V:%.1fV | Freq:%.2fHz | PhaseCorr:%d | Bill:$%.2f | Fan:%.1fh\n",
                   cycle, grid->bus_voltage, grid->grid_frequency, 
                   grid->phase_correction_active, util->total_billing_kwh * util->customer_rate_per_kwh,
                   util->cooling_fan_runtime);
        }

        usleep(4000); 
    }

    printf("\nSubstation Simulation Complete.\n");

    free(grid); free(util); free(safety);
    return 0;
}

void init_substation(GridState* g, UtilityMetrics* u, SafetyMonitor* s) {
    g->bus_voltage = 230.0f;
    g->grid_frequency = 60.0f;
    g->phase_correction_active = 0;
    g->fault_indicator = 0;

    for (int i = 0; i < ZONE_COUNT; i++) {
        g->breaker_rating[i] = 50.0f + (i * 25.0f);
        g->active_load_amps[i] = 10.0f;
        g->breaker_status[i] = 1;
        for (int j = 0; j < OVERLOAD_WINDOW; j++) g->load_history[i][j] = 0.0f;
    }

    u->total_billing_kwh = 0.0;
    u->customer_rate_per_kwh = 0.12f;
    u->packet_id = 0;
    u->operator_privilege = 1;
    u->cooling_fan_runtime = 0.0f;
    u->panel_temperature = 35.0f;
    u->ambient_light_lux = 500;

    s->safety_violations = 0;
    s->grid_shutdown = false;
}

void shift_load_history(GridState* g) {
    for (int i = 0; i < ZONE_COUNT; i++) {
        for (int j = 0; j < OVERLOAD_WINDOW - 1; j++) {
            g->load_history[i][j] = g->load_history[i][j+1];
        }
        g->load_history[i][OVERLOAD_WINDOW - 1] = g->active_load_amps[i];
    }
}

void simulate_power_flow(GridState* g) {
    // Voltage fluctuations
    g->bus_voltage = 225.0f + (float)(rand() % 10);
    
    // Frequency drift
    g->grid_frequency += ((float)(rand() % 100) - 50.0f) * 0.01f;

    // Load variation in each zone
    for (int i = 0; i < ZONE_COUNT; i++) {
        if (g->breaker_status[i]) {
            // Occasionally simulate a surge
            if (rand() % 100 < 5) g->active_load_amps[i] += 40.0f;
            else g->active_load_amps[i] = 20.0f + (rand() % 15);
        } else {
            g->active_load_amps[i] = 0.0f;
        }
    }
}

void grid_controller(GridState* g) {
    // Frequency control
    if (fabs(g->grid_frequency - NOMINAL_FREQ) > 0.5f) {
        g->phase_correction_active = 1;
    } else {
        g->phase_correction_active = 0;
    }

    // Manual Reset Logic for Faults
    if (rand() % 200 == 0) g->fault_indicator = 0;
}

bool verify_substation_integrity(GridState* g, SafetyMonitor* s) {
    bool safe = true;

    // --- Invariant 1: Temporal Overcurrent Protection ---
    for (int i = 0; i < ZONE_COUNT; i++) {
        bool sustained_overload = true;
        for (int j = 0; j < OVERLOAD_WINDOW; j++) {
            if (g->load_history[i][j] <= g->breaker_rating[i]) {
                sustained_overload = false;
                break;
            }
        }

        if (sustained_overload && g->breaker_status[i] == 1) {
            printf("[SAFETY] Zone %d breaker failed to trip after sustained overload!\n", i);
            safe = false;
        }
    }

    // --- Invariant 2: Impedance Fault Detection ---
    for (int i = 0; i < ZONE_COUNT; i++) {
        if (g->breaker_status[i] == 1 && g->active_load_amps[i] > 1.0f) {
            float impedance = g->bus_voltage / g->active_load_amps[i];
            if (impedance < SHORT_CIRCUIT_IMPEDANCE) {
                if (g->fault_indicator == 0) {
                    printf("[SAFETY] Fault undetected! Low impedance (%.2f) on Zone %d\n", impedance, i);
                    safe = false;
                }
            }
        }
    }

    // --- Invariant 3: Frequency Stability ---
    if (fabs(g->grid_frequency - NOMINAL_FREQ) > FREQ_TOLERANCE) {
        if (g->phase_correction_active == 0) {
            printf("[SAFETY] PLL Failure! High freq deviation (%.2f) without correction.\n", g->grid_frequency);
            safe = false;
        }
    }

    return safe;
}

// --- NON-CONDITIONALLY RELEVANT FUNCTIONS (Non-CRVs) ---

void update_utility_billing(UtilityMetrics* u, GridState* g) {
    float current_power = 0;
    for (int i = 0; i < ZONE_COUNT; i++) {
        current_power += (g->bus_voltage * g->active_load_amps[i]) / 1000.0f; // kW
    }
    
    // Billing variables change but never influence the safety invariants
    u->total_billing_kwh += (current_power * 0.001);
    
    if (u->total_billing_kwh > 500.0) {
        u->customer_rate_per_kwh = 0.15f; // Peak pricing logic
    }
}

void run_network_diagnostics(UtilityMetrics* u) {
    u->packet_id++;
    
    // Computational "busy work" using bitwise and hashing on non-CRVs
    unsigned long hash = 5381;
    hash = ((hash << 5) + hash) + u->packet_id;
    hash = ((hash << 5) + hash) + u->operator_privilege;
    u->communication_hash = hash;

    if (u->packet_id % 100 == 0) {
        u->ambient_light_lux = 200 + (rand() % 600);
    }
}

void maintenance_subroutine(UtilityMetrics* u, GridState* g) {
    // Fan logic: Runs if panel temp is high. 
    // Neither fan runtime nor panel temp are used in the substation safety logic.
    u->panel_temperature = 30.0f + (g->bus_voltage * 0.02f);
    
    if (u->panel_temperature > 40.0f) {
        u->cooling_fan_runtime += 0.1f;
    }

    // Operator session management
    if (u->cooling_fan_runtime > 100.0f) {
        u->operator_privilege = 0; // Lockout for service
    }
}

void auxiliary_reporting(UtilityMetrics* u) {
    // Large loop to satisfy code length and create deep paths for static analysis
    for (int i = 0; i < 50; i++) {
        if (u->ambient_light_lux < 100) {
            u->communication_hash ^= 0xFFFFFFFF;
        } else {
            u->communication_hash &= 0xAAAAAAAA;
        }
    }
}
