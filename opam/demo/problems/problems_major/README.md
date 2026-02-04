# Conditionally Relevant Variable (CRV) Detection Benchmarks

This directory contains 6 non-trivial C programs designed as benchmarks for the task of detecting **Conditionally Relevant Variables (CRVs)**. 

In formal verification, a variable is "conditionally relevant" if its value can influence the truth value of a program's safety assertions. These programs are structured to contain complex control logic and a mix of variables:
1. **Safety-Critical Variables:** Variables directly or indirectly involved in the safety assertions.
2. **Non-CRVs:** Variables (e.g., UI settings, logging IDs, environmental noise) that are frequently updated but have no logical path to affect the safety conditions.

## Program Descriptions and Safety Conditions

### 1. `car_simulation.c`
Simulates a vehicle journey including gear management, acceleration physics, and a history-based speed window.
*   **Safety Condition:** 
    *   The current speed must never exceed the maximum legal limit.
    *   If the car is in Gear 2 and the speed falls within a specific "stability range," the instantaneous acceleration must not exceed a predefined window limit.
    *   The gear must be at least 3 if the average speed over the last 10 cycles is greater than 60 km/h.

### 2. `chemical_reactor.c`
Simulates an automated batch reactor where exothermic reactions must be kept within thermal and pressure bounds.
*   **Safety Condition:** 
    *   If internal pressure exceeds 150 PSI, the emergency relief vent must be open.
    *   If reactant concentration is high (> 80%), the temperature increase between two cycles must be less than 8.0K.
    *   If the cooling pump has been at 100% capacity for 5 or more cycles, the temperature history window (T0 to T4) must not be strictly increasing.

### 3. `drone_delivery.c`
Simulates a delivery quadcopter navigating wind resistance, battery drain, and shifting payloads.
*   **Safety Condition:** 
    *   At high altitudes (> 20m) in high wind (> 15m/s), the pitch angle must stay within a stable range (±15 degrees).
    *   If the battery falls below 15% and the drone is in the air, the vertical velocity must be non-positive (descending or landed).
    *   If the payload weight fluctuates by more than 0.5kg in a 5-step window, the throttle input must not increase.

### 4. `medical_infusion_pump.c`
Simulates a medical device delivering intravenous medication with bubble and occlusion detection.
*   **Safety Condition:** 
    *   The pump must shut off if the air sensor detects bubbles for two consecutive cycles.
    *   The pump must shut off if downstream pressure exceeds 15.0 PSI (occlusion detected).
    *   For any contiguous part of the history where the delivered dose is above 95% of the limit, the flow rate must be non-increasing within those specific contiguous segments.

### 5. `smart_grid_substation.c`
Simulates an electrical substation balancing loads and protecting grid hardware.
*   **Safety Condition:** 
    *   A breaker must be open if its zone has been in an overcurrent state (load > rating) for 3 consecutive steps.
    *   A fault indicator must be active if a closed breaker detects a low-impedance short circuit (Voltage/Load < 0.5).
    *   Phase correction must be active if the grid frequency deviates from 60Hz by more than 0.8Hz.

### 6. `warehouse_robot.c`
Simulates an autonomous mobile robot (AGV) transporting heavy loads through a warehouse.
*   **Safety Condition:** 
    *   The robot must stay below 2.0 m/s if an obstacle is within 5.0 meters.
    *   If carrying a heavy load (> 500kg) while turning, motor torque must be capped at 40.0 Nm to prevent tipping.
    *   If the robot's speed has been strictly decreasing for 3 consecutive cycles, the brake pressure must be active (> 0).

## Experimental Usage
Each file is at least 200 lines of code and utilizes structs and primitive types. They are to be run using the "-lm" flag.
