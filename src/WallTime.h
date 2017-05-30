#ifndef KV_FS_WALLTIME_H
#define KV_FS_WALLTIME_H

#include <sys/time.h>
#include <time.h>
#include <stdlib.h>


/**
 * The WallTime class provides a fast, seconds-granularity real time clock.
 * It works by initially querying the OS for the initial time value, and then
 * increments that count based on the CPU's timestamp counter in subsequent
 * calls.
 *
 * WallTime clocks are 32-bit, like Unix's time_t, though it's unsigned and the
 * "RAMCloud" epoch begins much more recently. This should last us until about
 * the year 2150. 
 */
class WallTime {
  public:
    static uint32_t secondsTimestamp(void);
    static time_t secondsTimestampToUnix(uint32_t timestamp);

    /**
     * The RAMCloud epoch began Jan 1 00:00:00 2011 UTC.
     * The Unix epoch began 41(!) years prior. May ours
     * last just as long!
     */
    static const uint32_t RAMCLOUD_UNIX_OFFSET = (41 * 365 * 86400);

    /**
     * The base Unix time, used by the timestamp methods below.
     * This exists so that we need only call the kernel once to
     * get the time. In the future, we'll use rdtsc().
     */
    static time_t baseTime;

    /**
     * CPU timestamp counter value at the first call to
     * #secondsTimestamp. Used to avoid subsequent syscalls
     * to obtain current time in conjunction with #baseTime.
     */
    static uint64_t baseTsc;

};

#endif

