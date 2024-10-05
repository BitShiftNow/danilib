// Danilib - dani_profiler.h
// Types and functions for profiling C code.
//
// Author: Dani Drywa (dani@drywa.me)
// This library is based on what I learned from Casey Muratori's excellent performance aware programming course at https://www.computerenhance.com/ and some other resources about benchmarking.
//
// Last change: 2024/10/05 (yyyy/mm/dd)
//
// License: See end of file
//
// Dependencies:
// dani_base.h - for the basic types
// Windows.h - for QueryPerformanceCounter and QueryPerformanceFrequency
// Intrin.h - for __rdtsc, __rdtscp, __faststorefence, and _InterlockedIncrement
// stdio.h - for printf. (can be removed by specifying DANI_PROFILER_PRINTF)
// psapi.h - for GetProcessMemoryInfo if DANI_PROFILER_PAGE_FAULTS is enabled.
//
// Notes:
// This library is *NOT* thread safe. If you are in need of profiling across multiple threads you have to make this code thread safe or you might want to consider using a different library better suited for your needs.
// All dependencies must be included before including this file.
// To include the implementation specify DANI_LIB_PROFILER_IMPLEMENTATION before including this file.
// To use static versions of the functions specify DANI_PROFILER_STATIC before including this file.
// By default the profiles is able to record up to 1024 entries. If you want to tweak this value specify DANI_PROFILER_ENTRIES_MAX before including this file.
// To print the profiling results this library is using printf by default. However, you can change the print function by specifying DANI_PROFILER_PRINTF(...) before including this file.
// To enable or disable the profiler define DANI_PROFILER_ENABLED with 1 or 0 respectively. By default the profiler is disabled. All profiling functions and macros will be stubbed out, with the exception of dani_BeginProfiling, dani_EndProfiling, and dani_PrintProfilingResults. These will still work and simply collect the overall elapsed time of the program without any sub-blocks.
// To collect memory page fault metrics set DANI_PROFILER_PAGE_FAULTS to 1. This even takes effect if DANI_PROFILER_ENABLED is set to 0, it will just collect page faults for the overall runtime. Memory page faults are always an inclusive count, which means they include all page faults of sub-zones as well.
// To enable collecting of min and max values set DANI_PROFILER_MIN_MAX to 1.
// To enable everything the profiler has to offer you can define DANI_PROFILER_ENABLE_ALL before including this library. This will overwrite any previous settings and enable everything.
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
// dani_profiler_zone my_zone = dani_BeginProfilingZone("Zone Name", index, 0); 
// // Add your code you want to profile here
// dani_EndProfilingZone(my_zone);
//
// If the profiling zone will be called multiple times it is best to use a local static variable for the zone index:
//
// static u32 local_index = 0;
// if (local_index == 0) {
//     local_index = dani_GetNextProfilerZoneIndex();
// }
// // Start a zone like usual...
//
// This pattern can be simplified by using the dani_Profile macro.
//
// dani_Profile(var_name, "Zone Name");
// // Add your code you want to profile here
// dani_ProfileEnd(var_name);
//
// The var_name in the macro will be used to create variables for the index and the zone.
// If you want to profile a whole function, consider the dani_ProfileFunction macro which uses __func__ as the zone name.
//
// void MyFunc(void) {
//      dani_ProfileFunction();
//      // Add your function code to profile here
//      dani_ProfileFunctionEnd();
// }
//
// dani_ProfileFunctionEnd should always be called before any return statements.
//
// The last argument of dani_BeginProfilingZone is the number of bytes that are about to be processed by the profiling zone. This can remain 0 if you don't want to track the bandwidth. To profile with the bandwidth included you have to provide the byte count. You can use the dani_ProfileBandwidth and dani_ProfileFunctionBandwidth macros in the same way you would use the dani_Profile and dani_ProfileFunction macros. The only difference is that it takes in a byte count argument as well.
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

#ifdef DANI_PROFILER_ENABLE_ALL
#define DANI_PROFILER_ENABLED 1
#define DANI_PROFILER_PAGE_FAULTS 1
#define DANI_PROFILER_MIN_MAX 1
#endif // DANI_PROFILER_ENABLE_ALL

#ifndef DANI_PROFILER_ENABLED
#define DANI_PROFILER_ENABLED 0
#endif

#ifndef DANI_PROFILER_PAGE_FAULTS
#define DANI_PROFILER_PAGE_FAULTS 0
#endif

#ifndef DANI_PROFILER_MIN_MAX
#define DANI_PROFILER_MIN_MAX 0
#endif

#if DANI_PROFILER_ENABLED

typedef struct __DANI_PROFILER_ENTRY dani_profiler_entry;
struct __DANI_PROFILER_ENTRY {
    u64 inclusive_ticks;
    u64 exclusive_ticks;

    u64 hit_counter;
    u64 processed_bytes_counter;

#if DANI_PROFILER_PAGE_FAULTS
    u64 page_fault_counter;
#endif // DANI_PROFILER_PAGE_FAULTS

#if DANI_PROFILER_MIN_MAX
    u64 inclusive_ticks_min;
    u64 inclusive_ticks_max;
#endif // DANI_PROFILER_MIN_MAX

    const s8 *name;
};

typedef struct __DANI_PROFILER_ZONE dani_profiler_zone;
struct __DANI_PROFILER_ZONE {
    const s8 *name;

    u64 start_ticks;
    u64 inclusive_ticks;

#if DANI_PROFILER_PAGE_FAULTS
    u64 start_page_faults;
#endif // DANI_PROFILER_PAGE_FAULTS

    u32 entry_index;
    u32 parent_index;
};

typedef struct __DANI_PROFILER dani_profiler;
struct __DANI_PROFILER {
    dani_profiler_entry entries[DANI_PROFILER_ENTRIES_MAX];

    u64 start_ticks;
    u64 end_ticks;

#if DANI_PROFILER_PAGE_FAULTS
    u64 start_page_faults;
    u64 end_page_faults;
#endif // DANI_PROFILER_PAGE_FAULTS

    u32 current_index;
};

__DANI_PROFILER_DEC void dani_BeginProfiling(void);
__DANI_PROFILER_DEC void dani_EndProfiling(void);
__DANI_PROFILER_DEC void dani_PrintProfilingResults(void);

__DANI_PROFILER_DEC u32 dani_GetNextProfilerZoneIndex(void);
__DANI_PROFILER_DEC dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index, u64 byte_count);
__DANI_PROFILER_DEC void dani_EndProfilingZone(dani_profiler_zone zone);

#define dani_ProfileBandwidth(var_name, zone_name, byte_count) \
    static u32 __dani_profile_##var_name##_index = 0;\
    if (__dani_profile_##var_name##_index == 0) {\
        __dani_profile_##var_name##_index = dani_GetNextProfilerZoneIndex();\
    }\
    dani_profiler_zone __dani_profile_##var_name##_zone = dani_BeginProfilingZone((zone_name), __dani_profile_##var_name##_index, (byte_count))

#define dani_Profile(var_name, zone_name) dani_ProfileBandwidth(var_name, zone_name, 0)
#define dani_ProfileEnd(var_name) dani_EndProfilingZone(__dani_profile_##var_name##_zone)

#define dani_ProfileFunction() dani_Profile(function, __func__)
#define dani_ProfileFunctionBandwidth(byte_count) dani_ProfileBandwidth(function, __func__, byte_count)
#define dani_ProfileFunctionEnd() dani_ProfileEnd(function)

#else // NOT DANI_PROFILER_ENABLED

typedef u64 dani_profiler_zone; 
typedef struct __DANI_PROFILER dani_profiler;
struct __DANI_PROFILER {
    u64 start_ticks;
    u64 end_ticks;

#if DANI_PROFILER_PAGE_FAULTS
    u64 start_page_faults;
    u64 end_page_faults;
#endif // DANI_PROFILER_PAGE_FAULTS
};

__DANI_PROFILER_DEC void dani_BeginProfiling(void);
__DANI_PROFILER_DEC void dani_EndProfiling(void);
__DANI_PROFILER_DEC void dani_PrintProfilingResults(void);

#define dani_GetNextProfilerZoneIndex() 0
#define dani_BeginProfilingZone(...) 0
#define dani_EndProfilingZone(...)

#define dani_ProfileBandwidth(...)
#define dani_Profile(...)
#define dani_ProfileEnd(...)

#define dani_ProfileFunction()
#define dani_ProfileFunctionBandwidth(...)
#define dani_ProfileFunctionEnd()

#endif // DANI_PROFILER_ENABLED
#endif // __DANI_LIB_PROFILER_H

#ifdef DANI_LIB_PROFILER_IMPLEMENTATION

static u64 ReadOSTimer(void) {
    LARGE_INTEGER timer;
    QueryPerformanceCounter(&timer);

    u64 result = timer.QuadPart;
    return (result);
}

static u64 ReadOSTimerFrequency(void) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    u64 result = frequency.QuadPart;
    return (result);
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

#if DANI_PROFILER_PAGE_FAULTS
static void *g_dani_profiler_process_handle = 0;

static u64 ReadOSPageFaultCount(void) {
    PROCESS_MEMORY_COUNTERS memory_counters = {0};
    memory_counters.cb = sizeof(memory_counters);

    u64 result = 0;
    b32 is_success = GetProcessMemoryInfo(g_dani_profiler_process_handle, &memory_counters, sizeof(memory_counters));
    if (is_success) {
        result = memory_counters.PageFaultCount;
    }

    return (result);
}

static void InitialiseOSProfilingMetrics(void) {
    if (g_dani_profiler_process_handle == 0) {
        u32 process_id = GetCurrentProcessId();
        g_dani_profiler_process_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, 0, process_id);
    }
}
#endif // DANI_PROFILER_PAGE_FAULTS

static dani_profiler g_dani_profiler = {0};

__DANI_PROFILER_DEF void dani_BeginProfiling(void) {
    // Initialise profiling metrics if enabled
#if DANI_PROFILER_PAGE_FAULTS
    InitialiseOSProfilingMetrics();
#endif // DANI_PROFILER_PAGE_FAULTS

    // Reset global profiler in case it has been used before
    memset(&g_dani_profiler, 0, sizeof(g_dani_profiler));

    // Warmup profiler
    ReadStartCPUTimer();
    ReadStartCPUTimer();
    ReadStartCPUTimer();
    ReadEndCPUTimer();

    // Start profiler
#if DANI_PROFILER_PAGE_FAULTS
    g_dani_profiler.start_page_faults = ReadOSPageFaultCount();
#endif // DANI_PROFILER_PAGE_FAULTS

    g_dani_profiler.start_ticks = ReadStartCPUTimer();
}

__DANI_PROFILER_DEF void dani_EndProfiling(void) {
    g_dani_profiler.end_ticks = ReadEndCPUTimer();

#if DANI_PROFILER_PAGE_FAULTS
    g_dani_profiler.end_page_faults = ReadOSPageFaultCount();
#endif // DANI_PROFILER_PAGE_FAULTS
}

static void PrintProfilingTimes(u64 elapsed_ticks, u64 cpu_frequency) {
    f64 seconds = ((f64)elapsed_ticks / (f64)cpu_frequency);
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
    } else if (seconds >= 60.0) {
        f64 minutes = (seconds / 60.0);
        if (minutes >= 60.0) {
            f64 hours = (minutes / 60.0);
            DANI_PROFILER_PRINTF("%0.4fh", hours);
        } else {
            DANI_PROFILER_PRINTF("%0.4fmin", minutes);
        }
    } else {
        DANI_PROFILER_PRINTF("%0.4fs", seconds);
    }
}

static void PrintProfilingValueAsSIUnit(f64 value, const s8 *base_unit) {
    s8 prefix;

    if (value >= Tera(1)) { prefix = 'T'; value /= Tera(1); }
    else if (value >= Giga(1)) { prefix = 'G'; value /= Giga(1); }
    else if (value >= Mega(1)) { prefix = 'M'; value /= Mega(1); }
    else if (value >= Kilo(1)) { prefix = 'k'; value /= Kilo(1); }
    else { prefix = '\0'; }

    u64 int_value = (u64)value;
    if ((f64)int_value == value) {
        // No fractions
        DANI_PROFILER_PRINTF("%llu%c%s", int_value, prefix, base_unit);
    } else {
        // Fractions
        DANI_PROFILER_PRINTF("%0.2f%c%s", value, prefix, base_unit);
    }
}

static void PrintProfilingByteCount(f64 byte_count) {
    s8 *prefix;

    if (byte_count >= TiB(1)) { prefix = "TiB"; byte_count /= TiB(1); }
    else if (byte_count >= GiB(1)) { prefix = "GiB"; byte_count /= GiB(1); }
    else if (byte_count >= MiB(1)) { prefix = "MiB"; byte_count /= MiB(1); }
    else if (byte_count >= KiB(1)) { prefix = "KiB"; byte_count /= KiB(1); }
    else { prefix = "byte"; }

    u64 int_value = (u64)byte_count;
    if ((f64)int_value == byte_count) {
        // No fractions
        DANI_PROFILER_PRINTF("%llu%s", int_value, prefix);
    } else {
        // Fractions
        DANI_PROFILER_PRINTF("%0.2f%s", byte_count, prefix);
    }
}

#if DANI_PROFILER_ENABLED

static void PrintInclusiveAndExclusiveProfilingTimes(u64 elapsed_inclusive, u64 elapsed_exclusive, u64 elapsed_total, u64 cpu_frequency) {
    f64 inclusive_percentage = ((f64)elapsed_inclusive / (f64)elapsed_total) * 100.0;

    if (elapsed_inclusive == elapsed_exclusive) {
        DANI_PROFILER_PRINTF("Incl/Excl[%0.2f%%]: ", inclusive_percentage);
        PrintProfilingTimes(elapsed_inclusive, cpu_frequency);
    } else {
        DANI_PROFILER_PRINTF("Incl[%0.2f%%]: ", inclusive_percentage);
        PrintProfilingTimes(elapsed_inclusive, cpu_frequency);

        f64 exclusive_percentage = ((f64)elapsed_exclusive / (f64)elapsed_total) * 100.0;

        DANI_PROFILER_PRINTF(", Excl[%0.2f%%]: ", exclusive_percentage);
        PrintProfilingTimes(elapsed_exclusive, cpu_frequency);
    }
}

static void PrintProfilingBandwidth(f64 processed_bytes_count, u64 elapsed_inclusive, u64 cpu_frequency) {
    f64 ticks_per_second = ((f64)elapsed_inclusive / (f64)cpu_frequency);
    f64 bytes_per_second = (processed_bytes_count / ticks_per_second);

    DANI_PROFILER_PRINTF(", Bandwidth[");
    PrintProfilingByteCount(processed_bytes_count);
    DANI_PROFILER_PRINTF("]: ");
    PrintProfilingByteCount(bytes_per_second);
    DANI_PROFILER_PRINTF("/s");
}

static void PrintInclusiveMinAndMaxProfilingTimes(u64 elapsed_min, u64 elapsed_max, u64 elapsed_total, u64 cpu_frequency) {
    f64 percentage_min = ((f64)elapsed_min / (f64)elapsed_total) * 100.0;

    DANI_PROFILER_PRINTF("Min[%0.2f%%]: ", percentage_min);
    PrintProfilingTimes(elapsed_min, cpu_frequency);

    f64 percentage_max = ((f64)elapsed_max / (f64)elapsed_total) * 100.0;

    DANI_PROFILER_PRINTF(", Max[%0.2f%%]: ", percentage_max);
    PrintProfilingTimes(elapsed_max, cpu_frequency);
}

static volatile s32 g_dani_profiler_entry_index_conter = 0;

__DANI_PROFILER_DEF u32 dani_GetNextProfilerZoneIndex(void) {
    u32 result = (u32)_InterlockedIncrement((volatile long *)&g_dani_profiler_entry_index_conter);
    Assert(result != 0 && result < DANI_PROFILER_ENTRIES_MAX);
    return (result);
}

__DANI_PROFILER_DEF dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index, u64 byte_count) {
    dani_profiler_zone result = {0};

    result.name = name;

    dani_profiler_entry *entry = &g_dani_profiler.entries[index];
    entry->processed_bytes_counter += byte_count;
    result.inclusive_ticks = entry->inclusive_ticks;

    result.entry_index = index;
    result.parent_index = g_dani_profiler.current_index;

    g_dani_profiler.current_index = index;
    
#if DANI_PROFILER_PAGE_FAULTS
    result.start_page_faults = ReadOSPageFaultCount();
#endif // DANI_PROFILER_PAGE_FAULTS

    result.start_ticks = ReadStartCPUTimer();
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
    entry->name = zone.name;

#if DANI_PROFILER_PAGE_FAULTS
    u64 end_page_faults = ReadOSPageFaultCount();
    entry->page_fault_counter = end_page_faults - zone.start_page_faults;
#endif // DANI_PROFILER_PAGE_FAULTS

#if DANI_PROFILER_MIN_MAX
    if (entry->hit_counter == 0) {
        entry->inclusive_ticks_min = elapsed_ticks;
        entry->inclusive_ticks_max = elapsed_ticks;
    } else {
        if (elapsed_ticks < entry->inclusive_ticks_min) {
            entry->inclusive_ticks_min = elapsed_ticks;
        }
        if (elapsed_ticks > entry->inclusive_ticks_max) {
            entry->inclusive_ticks_max = elapsed_ticks;
        }
    }
#endif // DANI_PROFILER_MIN_MAX

    entry->hit_counter += 1;

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
        PrintProfilingValueAsSIUnit((f64)cpu_frequency, "Hz");
        DANI_PROFILER_PRINTF("\n");

#if DANI_PROFILER_PAGE_FAULTS
        u64 total_page_faults = g_dani_profiler.end_page_faults - g_dani_profiler.start_page_faults;
        DANI_PROFILER_PRINTF("Total page faults: ");
        PrintProfilingValueAsSIUnit((f64)total_page_faults, "");
        DANI_PROFILER_PRINTF("\n");
#endif // DANI_PROFILER_PAGE_FAULTS

#if DANI_PROFILER_ENABLED
        for (u32 entry_index = 0; entry_index < ArrayCount(g_dani_profiler.entries); entry_index += 1) {
            dani_profiler_entry *entry = &g_dani_profiler.entries[entry_index];
            if (entry->inclusive_ticks) {
                // Total time
                DANI_PROFILER_PRINTF("  %s[", entry->name);
                PrintProfilingValueAsSIUnit((f64)entry->hit_counter, "");
                DANI_PROFILER_PRINTF("] Total - ");
                PrintInclusiveAndExclusiveProfilingTimes(entry->inclusive_ticks, entry->exclusive_ticks, elapsed_total_ticks, cpu_frequency);

                if (entry->processed_bytes_counter) {
                    PrintProfilingBandwidth((f64)entry->processed_bytes_counter, entry->inclusive_ticks, cpu_frequency);
                }

#if DANI_PROFILER_PAGE_FAULTS
                if (entry->page_fault_counter) {
                    DANI_PROFILER_PRINTF(", Page faults: ");
                    PrintProfilingValueAsSIUnit((f64)entry->page_fault_counter, "");
                }
#endif // DANI_PROFILER_PAGE_FAULTS

                // Average time
                if (entry->hit_counter > 1) {
                    u64 average_inclusive = entry->inclusive_ticks / entry->hit_counter;
                    u64 average_exclusive = entry->exclusive_ticks / entry->hit_counter;

                    DANI_PROFILER_PRINTF("\n    Average - ");
                    PrintInclusiveAndExclusiveProfilingTimes(average_inclusive, average_exclusive, elapsed_total_ticks, cpu_frequency);

                    if (entry->processed_bytes_counter) {
                        f64 average_bytes = ((f64)entry->processed_bytes_counter / (f64)entry->hit_counter);
                        PrintProfilingBandwidth(average_bytes, average_inclusive, cpu_frequency);
                    }

#if DANI_PROFILER_PAGE_FAULTS
                    if (entry->page_fault_counter) {
                        f64 average_page_faults = ((f64)entry->page_fault_counter / (f64)entry->hit_counter);
                        DANI_PROFILER_PRINTF(", Page faults: ");
                        PrintProfilingValueAsSIUnit(average_page_faults, "");
                    }
#endif // DANI_PROFILER_PAGE_FAULTS
                }

#if DANI_PROFILER_MIN_MAX
                // Max & max time
                if (entry->hit_counter > 1 && entry->inclusive_ticks_max) {
                    DANI_PROFILER_PRINTF("\n    Extreme - ");
                    PrintInclusiveMinAndMaxProfilingTimes(entry->inclusive_ticks_min, entry->inclusive_ticks_max, elapsed_total_ticks, cpu_frequency);
                }
#endif // DANI_PROFILER_MIN_MAX
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
