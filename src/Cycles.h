#ifndef KV_FS_CYCLES_H
#define KV_FS_CYCLES_H

#include <stdint.h>

/**
 * This class provides static methods that read the fine-grain CPU
 * cycle counter and translate between cycle-level times and absolute
 * times.
 */
class Cycles {
  public:
    static void init();

    /**
     * Return the current value of the fine-grain CPU cycle counter
     * (accessed via the RDTSC instruction).
     */
    //static __inline __attribute__((always_inline))
    static uint64_t
    rdtsc()
    {
        uint32_t lo, hi;
        __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
        return (((uint64_t)hi << 32) | lo);
    }

    static double toSeconds(uint64_t cycles, double cyclesPerSec = 0);
    static uint64_t fromSeconds(double seconds, double cyclesPerSec = 0);
    static uint64_t toMicroseconds(uint64_t cycles, double cyclesPerSec = 0);
    static uint64_t toNanoseconds(uint64_t cycles, double cyclesPerSec = 0);
    static uint64_t fromNanoseconds(uint64_t ns, double cyclesPerSec = 0);
    static void sleep(uint64_t us);

    static double cyclesPerSec;

};

#endif

