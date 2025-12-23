#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

/* SAFETY CONDITION
 * 	(rod_depth >= ROD_FULLY_WITHDRAWN && rod_depth <= ROD_FULLY_INSERTED)
 * &&
 *	(!scram_active || rod_depth == ROD_FULLY_INSERTED) 
 * &&
 *	!(coolant_temperature >= HIGH_TEMP_THRESHOLD && rod_depth < ROD_FULLY_INSERTED);
 */

// --- Constants ---
#define ROD_FULLY_INSERTED 100
#define ROD_FULLY_WITHDRAWN 0

#define HIGH_TEMP_THRESHOLD 800        // °C
#define TEMP_RATE_THRESHOLD 25         // °C per cycle
#define SCRAM_HOLD_CYCLES 3             // Minimum cycles to hold SCRAM

#define MAX_ROD_STEP 10                // Max change per cycle (%)

// --- Simulated Hardware Inputs ---
volatile int coolant_temperature;
volatile int last_coolant_temperature;
volatile bool seismic_event_detected;
volatile int power_grid_demand;         // MW (CRV candidate)

// --- Internal Safety State ---
volatile bool scram_active;
volatile int scram_timer;

// --- Sensor Simulation ---
void read_reactor_sensors() {
    last_coolant_temperature = coolant_temperature;
    coolant_temperature = 740 + (rand() % 120);     // 740–859 °C
    seismic_event_detected = (rand() % 25 == 0);    // 4% chance
    power_grid_demand = 500 + (rand() % 200);       // 500–699 MW
}

// --- Logging ---
void log_rod_position(const char* reason, int depth) {
    printf("Logic: %-30s | Rod Insertion: %3d%% | Temp: %d°C | SCRAM: %s\n",
           reason,
           depth,
           coolant_temperature,
           scram_active ? "YES" : "NO");
}

// --- Control Logic ---
int step_control_logic(int last_rod_depth) {
    int new_rod_depth = last_rod_depth;

    int temp_rate = coolant_temperature - last_coolant_temperature;

    // 1. SCRAM DETECTION (Latching)
    if (seismic_event_detected || coolant_temperature >= HIGH_TEMP_THRESHOLD) {
        scram_active = true;
        scram_timer = SCRAM_HOLD_CYCLES;
    }

    // 2. SCRAM OVERRIDE
    if (scram_active) {
        new_rod_depth = ROD_FULLY_INSERTED;
        scram_timer--;
        if (scram_timer <= 0 && coolant_temperature < HIGH_TEMP_THRESHOLD - 50 && !seismic_event_detected) {
            scram_active = false; // Allow recovery only after cooldown
        }
        log_rod_position("SCRAM OVERRIDE ACTIVE", new_rod_depth);
        return new_rod_depth;
    }

    // 3. THERMAL PROTECTION (No withdrawal if temp rising fast)
    if (temp_rate > TEMP_RATE_THRESHOLD) {
        new_rod_depth = last_rod_depth + MAX_ROD_STEP;
        log_rod_position("TEMP RISE LIMITING POWER", new_rod_depth);
    }
    // 4. NORMAL OPERATION (CRV-driven)
    else {
        int target_depth = 100 - (power_grid_demand / 10); // Inverse demand relation
        if (target_depth > last_rod_depth + MAX_ROD_STEP)
            new_rod_depth = last_rod_depth + MAX_ROD_STEP;
        else if (target_depth < last_rod_depth - MAX_ROD_STEP)
            new_rod_depth = last_rod_depth - MAX_ROD_STEP;
        else
            new_rod_depth = target_depth;

        log_rod_position("POWER DEMAND FOLLOWING", new_rod_depth);
    }

    // 5. FINAL SATURATION
    if (new_rod_depth > ROD_FULLY_INSERTED) new_rod_depth = ROD_FULLY_INSERTED;
    if (new_rod_depth < ROD_FULLY_WITHDRAWN) new_rod_depth = ROD_FULLY_WITHDRAWN;

    return new_rod_depth;
}

// --- Main ---
int main() {
    srand(time(NULL));

    int rod_depth = ROD_FULLY_INSERTED;
    coolant_temperature = 750;
    last_coolant_temperature = 750;
    scram_active = false;
    scram_timer = 0;

    printf("--- Nuclear Reactor Control Rod Simulation ---\n\n");

    for (int i = 0; i < 15; ++i) {
        read_reactor_sensors();
        rod_depth = step_control_logic(rod_depth);

        printf("\n");
    }

    return 0;
}

