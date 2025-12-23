#ifndef SIMULATE_SEU_H
#define SIMULATE_SEU_H

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

int nondet_int();

// Function to generate a nondeterministic integer within the range [1, 32]
int nondet_int_range_1_32() {
    int value = nondet_int() % 32 + 1;
    __CPROVER_assume(value >= 1 && value <= 32);
    return value;
}

int simulate_seu(int value, int bit_pos) {
    int mask = 1 << bit_pos;
    __CPROVER_assume(mask >= 1 && mask <= 32);
    return (value ^ mask); // XOR operation for bit flip
}

// Ensures that an SEU is introduced only once for the variable under investigation
void simulate_seu_main(int *invest_var) {
    static int count = 0;
    if(count == 0) {
        int bit_pos = nondet_int_range_1_32();
        *invest_var = simulate_seu(*invest_var, bit_pos);
        count++;
    }
}

#endif // SIMULATE_SEU_H
