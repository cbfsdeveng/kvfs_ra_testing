#include "Cycles.h"
#include "WallTime.h"

// Static member declarations and initial values.
time_t WallTime::baseTime = 0;
uint64_t WallTime::baseTsc = 0;

/**
 * Cast a bigger int down to a smaller one.
 * Asserts that no precision is lost at runtime.
 */
template<typename Small, typename Large>
Small
downCast(const Large& large)
{
    Small small = static_cast<Small>(large);
    // The following comparison (rather than "large==small") allows
    // this method to convert between signed and unsigned values.
    //assert(large-small == 0);
    return small;
}

/**
 * Obtain a fast timestamp with seconds granularity. This stamp is an offset
 * from Jan 1 00:00:00 2011 UTC. A syscall is used only on the first invocation.
 * All subsequent calls use CPUs timestamp counter.
 */
uint32_t
WallTime::secondsTimestamp()
{
    if (baseTime == 0) {
        baseTime = time(NULL);
        baseTsc = Cycles::rdtsc();

        if (baseTime == -1) {
            //fprintf(stderr, "ERROR: The time(3) syscall failed!!");
            
            exit(1);
        }

        if (baseTime < RAMCLOUD_UNIX_OFFSET) {
            //fprintf(stderr, "ERROR: Your clock is too far behind. "
            //    "Please fix the date on your system!");
            exit(1);
        }
    }

    // calculate seconds offset. be careful to round up.
    uint32_t tscSecondsOffset = downCast<uint32_t>(Cycles::toNanoseconds(
        Cycles::rdtsc() - baseTsc + 500000000) / 1000000000U);

    return downCast<uint32_t>(baseTime) - RAMCLOUD_UNIX_OFFSET +
        tscSecondsOffset;
}

/**
 * Convert a timestamp returned via #secondsTimestamp to a Unix time_t
 * value, with the appropriate offset from the Unix epoch.
 *
 * \param[in] timestamp
 *      A RAMCloud epoch timestamp, as returned via #secondsTimestamp. 
 */
time_t
WallTime::secondsTimestampToUnix(uint32_t timestamp)
{
    return (time_t)timestamp + RAMCLOUD_UNIX_OFFSET;
}


