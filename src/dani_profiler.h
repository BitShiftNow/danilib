// Danilib - dani_profiler.h
// Types and functions for profiling C code.
//
// Author: Dani Drywa (dani@drywa.me)
//
// Last change: 2024/09/22 (yyyy/mm/dd)
//
// License: See end of file
//
// Dependencies:
// dani_base.h - for the basic types
// Windows.h - for QueryPerformanceCounter and QueryPerformanceFrequency
// Intrin.h - for __rdtsc, __rdtscp, and __faststorefence
//
// Notes:
// This library is *NOT* thread safe. If you are in need of profiling across multiple threads you have to make this code thread safe or you might want to consider using a different library better suited for your needs.
// All dependencies must be included before including this file.
// To include the implementation specify DANI_LIB_PROFILER_IMPLEMENTATION before including this file.
// To use static versions of the functions specify DANI_PROFILER_STATIC before including this file.
// By default the profiles is able to record up to 1024 entries. If you want to tweak this value specify DANI_PROFILER_ENTRIES_MAX before including this file.
// To print the profiling results this library is using printf by default. However, you can change the print function by specifying DANI_PROFILER_PRINTF(...) before including this file.
//
// How to use:
// To start Profiling call dani_BeginProfiling();
// To stop profiling call dani_EndProfiling();
// To print profiling results call dani_PrintProfilingResults();
// Restarting profiling after calling dani_EndProfiling() will give invalid results. Consider a profiling runtime to also be the program runtime.
// To time multiple statements of code use dani_BeginProfilingZone() and dani_EndProfilingZone() like so:
// 
// u32 index = GetNextEntryIndex(); // The maximum valid index depends on DANI_PROFILER_ENTRIES_MAX.
// dani_profiler_zone my_zone = dani_BeginProfilingZone("Zone Name", index); 
// // Add your code you want to profile here
// dani_EndProfilingZone(my_zone);
//
// This pattern can be simplified by using the dani_Profile macro which uses __COUNTER__ as the index.
//
// dani_Profile(my_zone, "Zone Name", {
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

typedef struct DANI_PROFILER_ENTRY dani_profiler_entry;
struct DANI_PROFILER_ENTRY {
    u64 elapsed_ticks;
    u64 hit_counter;

    const s8 *name;
};

typedef struct DANI_PROFILER dani_profiler;
struct DANI_PROFILER {
    dani_profiler_entry entries[DANI_PROFILER_ENTRIES_MAX];

    u64 start_ticks;
    u64 end_ticks;
};

typedef struct DANI_PROFILER_ZONE dani_profiler_zone;
struct DANI_PROFILER_ZONE {
    const s8 *name;

    u64 start_ticks;
    u32 entry_index;
};

__DANI_PROFILER_DEC void dani_BeginProfiling(void);
__DANI_PROFILER_DEC void dani_EndProfiling(void);
__DANI_PROFILER_DEC void dani_PrintProfilingResults(void);

__DANI_PROFILER_DEC u32 GetNextEntryIndex(void);
__DANI_PROFILER_DEC dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index);
__DANI_PROFILER_DEC void dani_EndProfilingZone(dani_profiler_zone zone);

#define dani_Profile(var_name, zone_name, profile_block) Statement({\
    static u32 StringifyCombine(var_name,entry_index) = 0;\
    if (StringifyCombine(var_name,entry_index) == 0) {\
        StringifyCombine(var_name,entry_index) = GetNextEntryIndex();\
    }\
    dani_profiler_zone var_name = dani_BeginProfilingZone(zone_name, StringifyCombine(var_name,entry_index));\
    profile_block\
    dani_EndProfilingZone(var_name);\
    })

#define dani_ProfileFunction(profile_block) dani_Profile(StringifyCombine(__func__,_zone),__func__, profile_block)

#endif // __DANI_LIB_PROFILER_H

#ifdef DANI_LIB_PROFILER_IMPLEMENTATION

static struct DANI_PROFILER g_dani_profiler = {0};

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

static void PrintCPUFrequency(f64 cpu_frequency) {
    s8 *format = "%0.2f Hz";

    if (cpu_frequency > 1000.0) {
        cpu_frequency /= 1000.0;
        format = "%0.2f kHz";

        if (cpu_frequency > 1000.0) {
            cpu_frequency /= 1000.0;
            format = "%0.2f MHz";

            if (cpu_frequency > 1000.0) {
                cpu_frequency /= 1000.0;
                format = "%0.2f GHz";
            }
        }
    }

    DANI_PROFILER_PRINTF(format, cpu_frequency);
}

__DANI_PROFILER_DEF void dani_PrintProfilingResults(void) {
    u64 cpu_frequency = ReadCPUTimerFrequency(100);
    u64 elapsed_ticks = g_dani_profiler.end_ticks - g_dani_profiler.start_ticks;

    if (cpu_frequency) {
        DANI_PROFILER_PRINTF("Total time: ");
        PrintProfilingTimes(elapsed_ticks, cpu_frequency);
        DANI_PROFILER_PRINTF(" @ ");
        PrintCPUFrequency((f64)cpu_frequency);
        DANI_PROFILER_PRINTF("\n\n");

        for (u32 entry_index = 0; entry_index < ArrayCount(g_dani_profiler.entries); entry_index += 1) {
            dani_profiler_entry *entry = &g_dani_profiler.entries[entry_index];
            if (entry->elapsed_ticks) {
                f64 percentage = ((f64)entry->elapsed_ticks / (f64)elapsed_ticks) * 100.0;
                DANI_PROFILER_PRINTF("  %s[Hits: %llu, Percentage: %0.2f%%] Total time: ", entry->name, entry->hit_counter, percentage);

                // Total time
                PrintProfilingTimes(entry->elapsed_ticks, cpu_frequency);

                // Average time
                u64 average_ticks = entry->elapsed_ticks / entry->hit_counter;
                DANI_PROFILER_PRINTF("\n    Average time: ");
                PrintProfilingTimes(average_ticks, cpu_frequency);
                DANI_PROFILER_PRINTF("\n\n");
            }
        }
    } else {
        DANI_PROFILER_PRINTF("Total ticks: %llu (Failed to estimate CPU frequency!)\n", elapsed_ticks);
    }
}

static volatile s32 g_dani_profiler_entry_index_conter = 0;

__DANI_PROFILER_DEF u32 GetNextEntryIndex(void) {
    u32 result = (u32)_InterlockedIncrement((volatile long *)&g_dani_profiler_entry_index_conter);
    Assert(result != 0 && result < DANI_PROFILER_ENTRIES_MAX);
    return (result);
}

__DANI_PROFILER_DEF dani_profiler_zone dani_BeginProfilingZone(const s8 *name, u32 index) {
    dani_profiler_zone result = {0};

    result.name = name;
    result.start_ticks = ReadStartCPUTimer();
    result.entry_index = index;

    return (result);
}

__DANI_PROFILER_DEF void dani_EndProfilingZone(dani_profiler_zone zone) {
    u64 end_ticks = ReadEndCPUTimer();
    u64 elapsed_ticks = end_ticks - zone.start_ticks;

    dani_profiler_entry *entry = &g_dani_profiler.entries[zone.entry_index];
    entry->elapsed_ticks += elapsed_ticks;
    entry->hit_counter += 1;
    entry->name = zone.name;
}

#endif // DANI_LIB_PROFILER_IMPLEMENTATION

/*
Danilib - dani_profiler.h License:
---------------------------------------------------------------------------------
Copyright 2024 Dani Drywa (dani@drywa.me)

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

4. If the source or binary form was of value to you, consider donating to the creator at https://ko-fi.com/danicrunch

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------------
*/
