#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

/**
 * SAFETY CONDITION
 *
 * 1. Aircraft speed must remain within physical bounds
 * 2. Acceleration must be bounded
 * 3. Environmental values must remain valid
 * 4. Aircraft speed must be non-decreasing over the last WINDOW_SIZE steps
 *    when turbine speed is non-decreasing (trend consistency)
 */
/* SAFETY CONDITION (CODE):
    (aircraft_speed >= 0.0 &&
     aircraft_speed <= MAX_AIRCRAFT_SPEED) &&

    (abs(aircraft_speed - prev_speed) <= MAX_ACCELERATION) &&

    (air_temperature >= MIN_AIR_TEMP &&
     air_temperature <= MAX_AIR_TEMP) &&

    (air_density >= MIN_AIR_DENSITY &&
     air_density <= MAX_AIR_DENSITY) &&

    (air_pressure >= MIN_PRESSURE &&
     air_pressure <= MAX_PRESSURE) &&

    (
	iteration_cnt < WINDOW_SIZE ||
	(
	    speed_history[0] <= speed_history[1] &&
	    speed_history[1] <= speed_history[2] &&
	    speed_history[2] <= speed_history[3]
	)
    );
*/

#define MAX_TURBINE_SPEED 1500
#define MIN_TURBINE_SPEED 0

#define MAX_AIRCRAFT_SPEED 350.0   // m/s (approx Mach 1 at altitude)
#define MAX_ACCELERATION 25.0      // m/s per iteration
#define WINDOW_SIZE 4              // history window for monotonicity
#define MIN_AIR_TEMP 200.0         // Kelvin
#define MAX_AIR_TEMP 330.0
#define MIN_AIR_DENSITY 0.5
#define MAX_AIR_DENSITY 2.0
#define MIN_PRESSURE 50000.0       // Pa
#define MAX_PRESSURE 120000.0

/**
 * Computes next aircraft speed.
 * Incorporates turbine acceleration smoothing and environmental normalization.
 */
double step(double aircraft_speed,int old_turbine_speed,int turbine_speed,double air_pressure,double air_temperature,double air_density)
{
    // Smooth turbine speed changes to avoid unrealistically sharp acceleration
    int effective_turbine_speed =
        (old_turbine_speed + turbine_speed) / 2;

    // Aerodynamic contribution (simplified physics-inspired model)
    double thrust_factor =
        (effective_turbine_speed * air_pressure) /
        (air_temperature * air_density * 1e4);

    // Clamp acceleration
    if (thrust_factor > MAX_ACCELERATION)
        thrust_factor = MAX_ACCELERATION;
    if (thrust_factor < -MAX_ACCELERATION)
        thrust_factor = -MAX_ACCELERATION;

    aircraft_speed += thrust_factor;
    return aircraft_speed;
}

int main() {
    srand(time(NULL));

    int turbine_speed = 0;
    int old_turbine_speed = 0;
    double aircraft_speed = 0.0;

    // Environmental variables
    double air_pressure = 101325.0;
    double air_temperature = 288.15;
    double air_density = 1.225;

    // History buffer for safety analysis
    double speed_history[WINDOW_SIZE] = {0};
    int iteration_cnt = 0;

    printf("--- Aircraft Turbine Speed Control Simulation ---\n");

    while (iteration_cnt < 200) {
        iteration_cnt++;

        // Simulate turbine speed command
        turbine_speed = rand() % (MAX_TURBINE_SPEED + 1);

        // Simulate environment variation
        air_pressure = 101325 + (rand() % 20000 - 10000);
        air_temperature = 288.15 + (rand() % 80 - 40);
        air_density = 1.225 + (rand() % 80 - 40) / 100.0;

        double prev_speed = aircraft_speed;

        aircraft_speed = step(aircraft_speed,old_turbine_speed,turbine_speed,air_pressure,air_temperature,air_density);

        old_turbine_speed = turbine_speed;

	/*
        // Shift history window
        for (int i = 0; i < WINDOW_SIZE - 1; i++) {
            speed_history[i] = speed_history[i + 1];
        }
        speed_history[WINDOW_SIZE - 1] = aircraft_speed;
	*/


        printf("Iter %3d | Turbine: %4d | Speed: %7.2f m/s\n",iteration_cnt,turbine_speed,aircraft_speed);
    }

    return 0;
}

