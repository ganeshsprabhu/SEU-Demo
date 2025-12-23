#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

/* SAFETY CONDITION:
 * 	(pump_rate >= MIN_PUMP_RATE && pump_rate <= MAX_PUMP_RATE) 
 * &&
 * 	!(post_treatment_chlorine_ppm >= HIGH_CHLORINE_ALARM_PPM && pump_rate != MIN_PUMP_RATE) 
 * &&
 *	!(post_treatment_chlorine_ppm <= LOW_CHLORINE_ALARM_PPM && pump_rate == MIN_PUMP_RATE && system_enabled) 
 * &&
 * 	fabs((double)(pump_rate)) <= MAX_PUMP_RATE;
 */


// -------------------- Constants --------------------
#define MAX_PUMP_RATE 100
#define MIN_PUMP_RATE 0

#define TARGET_CHLORINE_PPM 2.0f
#define HIGH_CHLORINE_ALARM_PPM 4.0f
#define LOW_CHLORINE_ALARM_PPM 0.5f

#define MAX_RATE_STEP 10            // Max % change per iteration
#define MAX_FLOW_LPM 800.0f
#define MIN_FLOW_LPM 100.0f

#define HISTORY_LEN 5               // For trend checks

// -------------------- Simulated Inputs --------------------
volatile float water_flow_rate_lpm;
volatile float post_treatment_chlorine_ppm;
volatile bool system_enabled;
volatile bool dosing_enabled_by_schedule;

// -------------------- Utility --------------------
float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// -------------------- Sensor Simulation --------------------
void read_plant_sensors() {
    water_flow_rate_lpm = 300.0f + (rand() % 401);              // 300–700 LPM
    post_treatment_chlorine_ppm = 0.8f + (rand() % 60) / 10.0f; // 0.8–6.7 ppm
    system_enabled = (rand() % 10) > 1;                         // Mostly enabled
    dosing_enabled_by_schedule = (rand() % 10) > 2;             // Mostly allowed
}

// -------------------- Logging --------------------
void log_doser_state(const char* reason, int rate) {
    printf("Reason: %-30s | Pump Rate: %3d%% | Flow: %6.1f LPM | Cl: %4.2f ppm\n",
           reason, rate, water_flow_rate_lpm, post_treatment_chlorine_ppm);
}

// -------------------- Control Logic --------------------
int step(int last_rate, float history[], int hist_len) {
    int new_rate = last_rate;

    // -------- Safety Override: High Chlorine --------
    if (post_treatment_chlorine_ppm >= HIGH_CHLORINE_ALARM_PPM) {
        new_rate = MIN_PUMP_RATE;
        log_doser_state("HIGH CHLORINE OVERRIDE", new_rate);
    }
    // -------- Normal Operation --------
    else if (system_enabled && dosing_enabled_by_schedule) {
        // Feed-forward term (flow-based)
        float ff = (water_flow_rate_lpm / MAX_FLOW_LPM) * 60.0f;

        // Feedback correction (chlorine error)
        float error = TARGET_CHLORINE_PPM - post_treatment_chlorine_ppm;
        float fb = error * 20.0f;

        // Trend-based damping: if chlorine rising fast, slow dosing
        float trend = history[hist_len - 1] - history[0];
        float damping = (trend > 0.5f) ? -10.0f : 0.0f;

        float raw_rate = ff + fb + damping;
        raw_rate = clampf(raw_rate, MIN_PUMP_RATE, MAX_PUMP_RATE);

        // Rate limiting
        int delta = (int)raw_rate - last_rate;
        if (delta > MAX_RATE_STEP) delta = MAX_RATE_STEP;
        if (delta < -MAX_RATE_STEP) delta = -MAX_RATE_STEP;

        new_rate = last_rate + delta;
        log_doser_state("FF + FB + Trend Control", new_rate);
    }
    // -------- Disabled States --------
    else {
        new_rate = MIN_PUMP_RATE;
        log_doser_state("SYSTEM OR SCHEDULE DISABLED", new_rate);
    }

    // -------- Final Saturation --------
    new_rate = clampi(new_rate, MIN_PUMP_RATE, MAX_PUMP_RATE);
    return new_rate;
}

// -------------------- Main --------------------
int main() {
    srand(1);

    int pump_rate = 0;
    float chlorine_history[HISTORY_LEN] = {TARGET_CHLORINE_PPM};

    printf("--- Refined Chlorine Doser Control Simulation ---\n");

    for (int i = 0; i < 200; ++i) {
        read_plant_sensors();

        // Shift history
        for (int h = 0; h < HISTORY_LEN - 1; ++h)
            chlorine_history[h] = chlorine_history[h + 1];
        chlorine_history[HISTORY_LEN - 1] = post_treatment_chlorine_ppm;

        pump_rate = step(pump_rate, chlorine_history, HISTORY_LEN);

    }

    return 0;
}

