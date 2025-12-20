#include <stdio.h>
#include <stdlib.h>
#include<stdbool.h>
#include<math.h>

/* Safety Condition: 's_t' => current aircraft speed, 's_{t-1}' => previous iteration's aircraft speed, 's_{t-4}' => value 4 iterations ago.
 *(
 * 	(s_t >= MIN_AIRCRAFT_SPEED && s_t <= MAX_AIRCRAFT_SPEED)
 * &&
 * 	fabs(s_t - s_{t-1}) <= MAX_DELTA_SPEED
 * &&
 * 	(iter < 5 || s_t >= s_{t-4})
 * &&
 * 	(fabs(s_t - s_{t-1}) <= fabs(s_{t-1} - s_{t-4}) + 5.0);
 * )
 */

#define MAX_TURBINE_SPEED 1500
#define MIN_AIRCRAFT_SPEED 0.0
#define MAX_AIRCRAFT_SPEED 400.0
#define MAX_DELTA_SPEED 20.0

double step(double aircraft_speed, int turbine_speed,double air_pressure, double air_temperature, double air_density)
{
    double delta =(turbine_speed * air_pressure)/(air_temperature * air_density + 1e-6);

    //scale down to realistic change.
    delta *= 1e-5;

    /* damping to prevent runaway */
    aircraft_speed = 0.97 * aircraft_speed + delta;
    return aircraft_speed;
}

int main(){
    srand(42);

    double aircraft_speed = 0.0;
    double aircraft_speed_prev1 = 0.0;
    double aircraft_speed_prev4 = 0.0;

    int turbine_speed = 600;
    int turbine_speed_prev = turbine_speed;

    /* controller state */
    double target_speed = 120.0;
    double integral_error = 0.0;
    double prev_error = 0.0;

    /* environment (slowly varying, deterministic) */
    double air_pressure = 101325.0;
    double air_temperature = 288.15;
    double air_density = 1.225;

    int iter = 0;

    while (iter<1000) {
        iter++;

        /* --- PI controller for turbine_speed --- */
        double error = target_speed - aircraft_speed;
        integral_error += error * 0.05;

        double control =
            6.0 * error +
            0.4 * integral_error -
            0.3 * (error - prev_error);

        prev_error = error;

        turbine_speed =
            turbine_speed_prev + (int)round(control);

        if (turbine_speed < 0)
            turbine_speed = 0;
        if (turbine_speed > MAX_TURBINE_SPEED)
            turbine_speed = MAX_TURBINE_SPEED;

        turbine_speed_prev = turbine_speed;

        /* --- update aircraft speed --- */
        aircraft_speed = step(aircraft_speed,turbine_speed,air_pressure,air_temperature,air_density);
	//printf("aircraft_speed: %f\n", aircraft_speed);
    }
    return 0;
}
