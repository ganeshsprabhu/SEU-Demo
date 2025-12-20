#include <stdio.h>
#include <stdbool.h>

/* SAFETY CONDITION:
 * 	(motor_command == MOTOR_CMD_STOP || motor_command == MOTOR_CMD_MOVE)
 * &&
 * 	(!human_in_safety_zone || motor_command == MOTOR_CMD_STOP)
 * &&
 * 	(mode != EMERGENCY_STOP || motor_command == MOTOR_CMD_STOP)
 * &&
 * 	(mode != SLOW_START || motor_prev == MOTOR_CMD_STOP)
 * &&
 * 	!(motor_prev == MOTOR_CMD_STOP && motor_command == MOTOR_CMD_MOVE && human_in_safety_zone);
 */

#define MOTOR_CMD_STOP 0
#define MOTOR_CMD_MOVE 1

#ifndef __CPROVER__
#define __CPROVER_assert(cond, msg) \
    do { if (!(cond)) { printf("ASSERTION FAILED: %s\n", msg); return -1; } } while (0)
#endif

typedef enum {
    NORMAL,
    SLOW_START,
    EMERGENCY_STOP
} robot_mode_t;

/* Simulated hardware inputs */
volatile bool human_in_safety_zone;
volatile int program_path_step;
volatile int total_path_steps = 100;

int step_control_logic(int prev_motor_command,robot_mode_t *mode)
{
    int new_command = prev_motor_command;

    /* --- Mode transitions --- */
    if (human_in_safety_zone) {
        *mode = EMERGENCY_STOP;
    }
    else if (*mode == EMERGENCY_STOP && !human_in_safety_zone) {
        *mode = SLOW_START;
    }

    /* --- Control logic per mode --- */
    if (*mode == EMERGENCY_STOP) {
        /* Hard safety override */
        new_command = MOTOR_CMD_STOP;
    }
    else if (*mode == SLOW_START) {
        /* Require one full STOP cycle before moving */
        if (prev_motor_command == MOTOR_CMD_STOP) {
            new_command = MOTOR_CMD_MOVE;
            *mode = NORMAL;
        } else {
            new_command = MOTOR_CMD_STOP;
        }
    }
    else { /* NORMAL */
        if (program_path_step < total_path_steps) {
            new_command = MOTOR_CMD_MOVE;
            program_path_step++;
        } else {
            new_command = MOTOR_CMD_STOP;
        }
    }

    return new_command;
}

int main() {
    int motor_command = MOTOR_CMD_STOP;
    int motor_prev = MOTOR_CMD_STOP;
    robot_mode_t mode = NORMAL;

    program_path_step = 0;

    printf("--- Refined Welding Robot Controller ---\n");

    for (int iter = 0; iter < 200; iter++) {

        /* Deterministic safety scenario */
        if (iter >= 20 && iter < 40)
            human_in_safety_zone = true;   // force emergency stop
        else
            human_in_safety_zone = false;

        motor_prev = motor_command;
        motor_command = step_control_logic(motor_prev, &mode);

        printf("iter=%d human=%d mode=%d step=%d cmd=%d\n",iter, human_in_safety_zone, mode, program_path_step, motor_command);

        if (program_path_step >= total_path_steps)
            break;
    }

    return 0;
}
