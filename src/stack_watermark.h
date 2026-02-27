#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdio.h>
#include <stdint.h>

// Linker symbols defined by the Pico SDK’s linker script
extern char __StackBottom[];
extern char __StackTop[];
extern char __StackOneBottom[];
extern char __StackOneTop[];

#define STACK_PATTERN 0xDEADBEEF

static void fill_stack(char *bottom, char *top) {
    uint32_t *p = (uint32_t *)bottom;
    while (p < (uint32_t *)top) {
        *p++ = STACK_PATTERN;
    }
}

static size_t measure_stack(char *bottom, char *top) {
    uint32_t *p = (uint32_t *)bottom;
    while (p < (uint32_t *)top && *p == STACK_PATTERN) {
        p++;
    }
    return (size_t)((uintptr_t)top - (uintptr_t)p);
}

// Initialize watermark pattern for both cores
void init_stack_watermark(void) {
    fill_stack(__StackBottom, __StackTop);
    fill_stack(__StackOneBottom, __StackOneTop);
}

// Print high-water usage for both cores
void print_stack_usage(void) {
    size_t used_core0 = measure_stack(__StackBottom, __StackTop);
    size_t used_core1 = measure_stack(__StackOneBottom, __StackOneTop);

    printf("Stack usage (core 0): %u bytes used of %u\n",
           (unsigned)used_core0, (unsigned)(__StackTop - __StackBottom));
    printf("Stack usage (core 1): %u bytes used of %u\n",
           (unsigned)used_core1, (unsigned)(__StackOneTop - __StackOneBottom));
}

