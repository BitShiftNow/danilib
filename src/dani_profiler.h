// Danilib - dani_profiler.h
// Types and functions for profiling C code.
//
// Author: Dani Drywa (dani@drywa.me)
// This library is based on what I learned from Casey Muratori's excellent performance aware programming course at https://www.computerenhance.com/ and some other resources about benchmarking.
//
// Last change: 2024/09/24 (yyyy/mm/dd)
//
// License: See end of file
//
// Dependencies:
// dani_base.h - for the basic types
// Windows.h - for QueryPerformanceCounter and QueryPerformanceFrequency
// Intrin.h - for __rdtsc, __rdtscp, __faststorefence, and _InterlockedIncrement
// stdio.h - for printf. (can be removed by specifying DANI_PROFILER_PRINTF)
//
// Notes:
// This library is *NOT* thread safe. If you are in need of profiling across multiple threads you have to make this code thread safe or you might want to consider using a different library better suited for your needs.
// All dependencies must be included before including this file.
// To include the implementation specify DANI_LIB_PROFILER_IMPLEMENTATION before including this file.
// To use static versions of the functions specify DANI_PROFILER_STATIC before including this file.
// By default the profiles is able to record up to 1024 entries. If you want to tweak this value specify DANI_PROFILER_ENTRIES_MAX before including this file.
// To print the profiling results this library is using printf by default. However, you can change the print function by specifying DANI_PROFILER_PRINTF(...) before including this file.
// To enable to disable the profiles define DANI_PROFILER_ENABLED with 1 or 0 respectively. By default the profiler is disabled. all profiling functions and macros will be stubbed out, with the exception of dani_BeginProfiling, dani_EndProfiling, and dani_PrintProfilingResults. These will still work and simply collect the overall elapsed time of the program without any sub-blocks.
//
// How to use:
// Enable the profiler by defining "DANI_PROFILER_ENABLED 1" before including the library. See Notes for details.
// To start Profiling call dani_BeginProfiling();
// To stop profiling call dani_EndProfiling();
// To print profiling results call dani_PrintProfilingResults();
// Restarting profiling after calling dani_EndProfiling() will give invalid results. Consider a profiling runtime to also be the program runtime.
// To time multiple statements of code use dani_BeginProfilingZone() and dani_EndProfilingZone() like so:
// 
// u32 index = dani_GetNextProfilerZoneIndex(); // The maximum valid index depends on DANI_PROFILER_ENTRIES_MAX.
// dani_profiler_zone my_zone = dani_BeginProfilingZone("Zone Name", index); 
// // Add your code you want to profile here
// dani_EndProfilingZone(my_zone);
//
// This pattern can be simplified by using the dani_Profile macro.
//
// dani_Profile("Zone Name", {
//      // Add your code you want to profile here 
// });
//
// If you want to profile a whole function block consider the dani_ProfileFunction macro which uses __func__ as the name.
//
// void MyFunc(void) {
//      dani_ProfileFunction({
//          // Add your function code to profile here
//      });
// }
//
// If the function returns the declaration of the return value and the return statement should not be in the dani_ProfileFunction block.
//
#ifndef __DANI_LIB_PROFILER_H
#define __DANI_LIB_PROFILER_H

#ifdef DANI_PROFILER_STATIC
#define __DANI_PROFILER_DEC static
#define __DANI_PROFILER_DEF static
#else
#define __DANI_PROFILER_DEC extern
#define __DANI_PROFILER_DEF
#endif

#ifndef DANI_PROFILER_ENTRIES_MAX
#define DANI_PROFILER_ENTRIES_MAX 1024
#endif

#ifndef DANI_PROFILER_PRINTF
#define DANI_PROFILER_PRINTF(...) printf(__VA_ARGS__)
#endif

#ifndef DANI_PROFILER_ENABLED
#define DANI_PROFILER_ENABLED 0
#endif

#if DANI_PROFILER_ENABLED

typedef struct __DANI_PROFILER_ENTRY dani_profiler_entry;
struct __DANI_PROFILER_ENTRY {
    u64 inclusive_ticks;
    u64 exclusive_ticks;
    u64 hit_counter;

    const s8 *name;
};

typedef struct __DANI_PROFILER_ZONE dani_profiler_zone;
struct __DANI_PROFILER_ZONE {
    const s8 *name;

    u64 start_ticks;
    u64 inclusive_ticks;

    u32 entry_index;
    u32 parent_index;
};

typedef struct __DANI_PROFILER dani_profiler;
struct __DANI_PROFILER {
    dani_profiler_entry entries[DANI_PROFILER_ENTRIES_MAX];

    u64 start_ticks;
    u64 end_ticks;

    u32 current_index;
};

__DANI_PROFILER_DEC void dani_BeginProfiling(void);
__DANI_PROFILER_DEC void dani_EndProfiling(void);
__DANI_PROFILER_DEC void dani_PrintProfilingResults(void);

__DANI_PROFILER_DEC u32 dani_GetNextProfilerZoneIndex(void);
__DANI_PROFILER_DEC dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index);
__DANI_PROFILER_DEC void dani_EndProfilingZone(dani_profiler_zone zone);

#define dani_Profile(zone_name, profile_block) Statement({\
    static u32 __entry_index = 0;\
    if (__entry_index == 0) {\
        __entry_index = dani_GetNextProfilerZoneIndex();\
    }\
    dani_profiler_zone __profile_zone = dani_BeginProfilingZone(zone_name, __entry_index);\
    profile_block\
    dani_EndProfilingZone(__profile_zone);\
    })

#define dani_ProfileFunction(profile_block) dani_Profile(__func__, profile_block)

#else // NOT DANI_PROFILER_ENABLED

typedef u64 dani_profiler_zone; 
typedef struct __DANI_PROFILER dani_profiler;
struct __DANI_PROFILER {
    u64 start_ticks;
    u64 end_ticks;
};

__DANI_PROFILER_DEC void dani_BeginProfiling(void);
__DANI_PROFILER_DEC void dani_EndProfiling(void);
__DANI_PROFILER_DEC void dani_PrintProfilingResults(void);

#define dani_GetNextProfilerZoneIndex() 0
#define dani_BeginProfilingZone(...) 0
#define dani_EndProfilingZone(...)

#define dani_Profile(zone_name, profile_block) Statement({\
    Unused(zone_name);\
    profile_block\
    })

#define dani_ProfileFunction(profile_block) dani_Profile(__func__, profile_block)

#endif // DANI_PROFILER_ENABLED

#endif // __DANI_LIB_PROFILER_H

#ifdef DANI_LIB_PROFILER_IMPLEMENTATION

static u64 ReadOSTimer(void) {
    LARGE_INTEGER timer;
    QueryPerformanceCounter(&timer);
    return (timer.QuadPart);
}

static u64 ReadOSTimerFrequency(void) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return (frequency.QuadPart);
}

static u64 ReadStartCPUTimer(void) {
    __faststorefence();
    u64 result = __rdtsc();
    return (result);
}

static u64 ReadEndCPUTimer(void) {
    unsigned int aux;
    u64 result = __rdtscp(&aux);
    __faststorefence();
    return (result);
}

static u64 ReadCPUTimerFrequency(u64 wait_time_ms) {
    u64 os_frequency = ReadOSTimerFrequency(); // Counts per second
    u64 os_wait_time = os_frequency * wait_time_ms / 1000;

    u64 cpu_start = ReadStartCPUTimer();
    u64 os_start = ReadOSTimer();

    u64 os_end;
    u64 os_elapsed = 0;

    while (os_elapsed < os_wait_time) {
        os_end = ReadOSTimer();
        os_elapsed = os_end - os_start;
    }

    u64 cpu_end = ReadEndCPUTimer();

    u64 cpu_elapsed = cpu_end - cpu_start;
    u64 result = 0;

    if (os_elapsed)
        result = os_frequency * cpu_elapsed / os_elapsed;

    return (result);
}

static dani_profiler g_dani_profiler = {0};

__DANI_PROFILER_DEF void dani_BeginProfiling(void) {
    // Warmup profiler
    ReadStartCPUTimer();
    ReadStartCPUTimer();
    ReadStartCPUTimer();
    ReadEndCPUTimer();

    // Start profiler
    g_dani_profiler.start_ticks = ReadStartCPUTimer();
}

__DANI_PROFILER_DEF void dani_EndProfiling(void) {
    g_dani_profiler.end_ticks = ReadEndCPUTimer();
}

static void PrintProfilingTimes(u64 elapsed_ticks, u64 cpu_frequency) {
    f64 seconds = (f64)elapsed_ticks / (f64)cpu_frequency;
    if (seconds < 1.0) {
        f64 milliseconds = seconds * 1000.0;
        if (milliseconds < 1.0) {
            f64 microseconds = milliseconds * 1000.0;
            if (microseconds < 1.0) {
                f64 nanoseconds = microseconds * 1000.0;
                DANI_PROFILER_PRINTF("%0.4fs (%0.4fms, %0.4fus, %0.4fns)", seconds, milliseconds, microseconds, nanoseconds);
            } else {
                DANI_PROFILER_PRINTF("%0.4fs (%0.4fms, %0.4fus)", seconds, milliseconds, microseconds);
            }
        } else {
            DANI_PROFILER_PRINTF("%0.4fs (%0.4fms)", seconds, milliseconds);
        }
    } else {
        DANI_PROFILER_PRINTF("%0.4fs", seconds);
    }
}

static void PrintProfilingValueAsSIUnit(u64 value, const s8 *base_unit) {
    s8 prefix;

    f64 fval = (f64)value;
    if (value > Tera(1)) { prefix = 'T'; fval /= Tera(1); }
    else if (value > Giga(1)) { prefix = 'G'; fval /= Giga(1); }
    else if (value > Mega(1)) { prefix = 'M'; fval /= Mega(1); }
    else if (value > Kilo(1)) { prefix = 'k'; fval /= Kilo(1); }
    else { prefix = '\0'; }

    if (prefix) {
        DANI_PROFILER_PRINTF("%0.2f%c%s", fval, prefix, base_unit);
    } else {
        DANI_PROFILER_PRINTF("%llu%s", value, base_unit);
    }
}

#if DANI_PROFILER_ENABLED

static void PrintInclusiveAndExclusiveProfilingTimes(u64 elapsed_inclusive, u64 elapsed_exclusive, u64 elapsed_total, u64 cpu_frequency) {
    f64 inclusive_percentage = ((f64)elapsed_inclusive / (f64)elapsed_total) * 100.0;
    f64 exclusive_percentage = ((f64)elapsed_exclusive / (f64)elapsed_total) * 100.0;

    const s8 *format = "Incl[%0.2f%%]: ";
    if (inclusive_percentage < 1.0) {
        format = "Incl[%0.4f%%]: ";
    }
    DANI_PROFILER_PRINTF(format, inclusive_percentage);
    PrintProfilingTimes(elapsed_inclusive, cpu_frequency);

    format = ", Excl[%0.2f%%]: ";
    if (inclusive_percentage < 1.0) {
        format = ", Excl[%0.4f%%]: ";
    }
    DANI_PROFILER_PRINTF(format, exclusive_percentage);
    PrintProfilingTimes(elapsed_exclusive, cpu_frequency);
}

static volatile s32 g_dani_profiler_entry_index_conter = 0;

__DANI_PROFILER_DEF u32 dani_GetNextProfilerZoneIndex(void) {
    u32 result = (u32)_InterlockedIncrement((volatile long *)&g_dani_profiler_entry_index_conter);
    Assert(result != 0 && result < DANI_PROFILER_ENTRIES_MAX);
    return (result);
}

__DANI_PROFILER_DEF dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index) {
    dani_profiler_zone result = {0};

    result.name = name;

    result.start_ticks = ReadStartCPUTimer();
    result.inclusive_ticks = g_dani_profiler.entries[index].inclusive_ticks;

    result.entry_index = index;
    result.parent_index = g_dani_profiler.current_index;

    g_dani_profiler.current_index = index;
    return (result);
}

__DANI_PROFILER_DEF void dani_EndProfilingZone(dani_profiler_zone zone) {
    u64 end_ticks = ReadEndCPUTimer();
    u64 elapsed_ticks = end_ticks - zone.start_ticks;

    dani_profiler_entry *parent = &g_dani_profiler.entries[zone.parent_index];
    dani_profiler_entry *entry = &g_dani_profiler.entries[zone.entry_index];
    
    parent->exclusive_ticks -= elapsed_ticks;

    entry->inclusive_ticks = zone.inclusive_ticks + elapsed_ticks;
    entry->exclusive_ticks += elapsed_ticks;
    entry->hit_counter += 1;
    entry->name = zone.name;

    g_dani_profiler.current_index = zone.parent_index;
}

#endif // DANI_PROFILER_ENABLED

__DANI_PROFILER_DEF void dani_PrintProfilingResults(void) {
    u64 cpu_frequency = ReadCPUTimerFrequency(100);
    u64 elapsed_total_ticks = g_dani_profiler.end_ticks - g_dani_profiler.start_ticks;

    if (cpu_frequency) {
        DANI_PROFILER_PRINTF("Total time: ");
        PrintProfilingTimes(elapsed_total_ticks, cpu_frequency);
        DANI_PROFILER_PRINTF(" @ ");
        PrintProfilingValueAsSIUnit(cpu_frequency, "Hz");
        DANI_PROFILER_PRINTF("\n");

        #if DANI_PROFILER_ENABLED
        for (u32 entry_index = 0; entry_index < ArrayCount(g_dani_profiler.entries); entry_index += 1) {
            dani_profiler_entry *entry = &g_dani_profiler.entries[entry_index];
            if (entry->inclusive_ticks) {
                DANI_PROFILER_PRINTF("\n");

                // Total time
                DANI_PROFILER_PRINTF("  %s[", entry->name);
                PrintProfilingValueAsSIUnit(entry->hit_counter, "");
                DANI_PROFILER_PRINTF("] Total - ");
                PrintInclusiveAndExclusiveProfilingTimes(entry->inclusive_ticks, entry->exclusive_ticks, elapsed_total_ticks, cpu_frequency);

                // Average time
                u64 average_inclusive = entry->inclusive_ticks / entry->hit_counter;
                u64 average_exclusive = entry->exclusive_ticks / entry->hit_counter;

                DANI_PROFILER_PRINTF("\n    Average - ");
                PrintInclusiveAndExclusiveProfilingTimes(average_inclusive, average_exclusive, elapsed_total_ticks, cpu_frequency);

                DANI_PROFILER_PRINTF("\n");
            }
        }
        #endif // DANI_PROFILER_ENABLED
    } else {
        DANI_PROFILER_PRINTF("Total ticks: %llu (Failed to estimate CPU frequency!)\n", elapsed_total_ticks);
    }
}

#endif // DANI_LIB_PROFILER_IMPLEMENTATION

/*
Danilib - dani_profiler.h License:
---------------------------------------------------------------------------------
Copyright (c) 2024 Dani Drywa (dani@drywa.me)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
4. If you use this software in a product, a donation to the original author
   (https://ko-fi.com/danicrunch) would be appreciated but is not required.
---------------------------------------------------------------------------------
*/
