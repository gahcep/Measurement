/*C: 
        __GNUC__
        __GNUC_MINOR__
        __GNUC_PATCHLEVEL__
        __GNUG__ (same as '__GNUC__ && __cplusplus')

    Clang:
        = GCC macros + 
        __clang__
        __clang_major__
        __clang_minor__
        __clang_patchlevel__

    MSVS:
        _MSC_VER

*/

// Platform (only x86/x86-64 supported)
#if defined (__i386) || defined (_M_IX86)
#define ENV_x86
#elif defined (_M_X64) || defined (__x86_64__)
#define ENV_x64
#else
#define ENV_UNKNOWN
#endif

// Compiler (only vc, g++ and clang supported)
#if defined __clang__
#define ENV_CLANG
#elif defined (__GNUC__) || defined (__GNUG__)
#define ENV_GCC
#elif defined (_MSC_VER)
#define ENV_MSVS
#define WIN32_LEAN_AND_MEAN
#endif

// Planfortm-dependent includes
#if defined (ENV_MSVS)
#include <windows.h>
#include <intrin.h>
#pragma intrinsic(__rdtsc, __rdtscp, __cpuid);
#else
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <iostream>
#include <iomanip>

#include <ratio>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <ctime>

namespace measure {

    template <class Rep = long long>
    using nanoseconds = std::chrono::duration < Rep, std::ratio<1, 1000000000> > ;

    template <class Rep = long long>
    using microseconds = std::chrono::duration < Rep, std::ratio<1, 1000000> >;

    template <class Rep = long long>
    using milliseconds = std::chrono::duration < Rep, std::ratio<1, 1000> >;
    
    template <class Rep = long long>
    using seconds = std::chrono::duration < Rep, std::ratio<1, 1> >;
    
    template <class Rep = long long>
    using minutes = std::chrono::duration < Rep, std::ratio<60, 1> >;
    
    template <class Rep = long long>
    using hours = std::chrono::duration < Rep, std::ratio<3600, 1> >;

    /* One time measurement */
    template <class Period = nanoseconds<long long>>
    class timer
    {
    public:

        /* Measure time interval using C++ ISO <chrono> library */

        template <class F, class... Args>
        static auto measure_time_cplusplus(F func, Args&&... args) -> typename Period::rep
        {
            auto start = std::chrono::high_resolution_clock::now();

            func(std::forward<Args>(args)...);

            auto delta = std::chrono::duration_cast<Period>(
                std::chrono::high_resolution_clock::now() - start);

            return delta.count();
        }

        /* Measure wall time using native techniques:
                -- on Linux: clock_gettime()
                -- on Windows: QueryPerformanceFrequency & QueryPerformanceCounter
        */

        template <class F, class... Args>
        static auto measure_wall_time(F func, Args&&... args) -> typename Period::rep
        {
            double scale = static_cast<double>(Period::period::den) / Period::period::num;

#ifdef ENV_MSVS
            // Non UTC-synchronized time stamps (using QPC)
            LARGE_INTEGER StartingTime, EndingTime, Elapsed;
            LARGE_INTEGER Frequency;

            // Returns counts per second
            QueryPerformanceFrequency(&Frequency);
            QueryPerformanceCounter(&StartingTime);
#else
            int result;
            
            // Returns seconds and microseconds values 
            //timeval tv_start, tv_finish;
            //result = gettimeofday(&tv_start, NULL);
            //if (result != 0) return 0;

            // Returns seconds and nanoseconds values
            timespec ts_start, ts_finish;
            result = clock_gettime(CLOCK_MONOTONIC, &ts_start);
#endif
            func(std::forward<Args>(args)...);

#ifdef ENV_MSVS
            QueryPerformanceCounter(&EndingTime);
            Elapsed.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

            Elapsed.QuadPart *= scale;
            Elapsed.QuadPart /= Frequency.QuadPart;

            return static_cast<typename Period::rep>(Elapsed.QuadPart);
#else
            //result = gettimeofday(&tv_finish, NULL);
            //if (result != 0) return 0;

            result = clock_gettime(CLOCK_MONOTONIC, &ts_finish);
            if (result != 0) return 0;

            // Convert according to scale 
            double start = scale * (ts_start.tv_sec + static_cast<double>(ts_start.tv_nsec) * 0.000000001);
            double finish = scale * (ts_finish.tv_sec + static_cast<double>(ts_finish.tv_nsec) * 0.000000001);

            //double start = scale * (tv_start.tv_sec + static_cast<double>(tv_start.tv_usec) * 0.000001);
            //double finish = scale * (tv_finish.tv_sec + static_cast<double>(tv_finish.tv_usec) * 0.000001);

            return static_cast<typename Period::rep>(finish - start);
#endif
        }

        /* Measure CPU time using native techniques:
            -- on Linux: getrusage()
            -- on Windows: GetProcessTimes
        */

        template <class F, class... Args>
        static auto measure_cpu_time(F func, Args&&... args) -> typename Period::rep
        {
            double scale = (double)Period::period::den / Period::period::num;

#ifdef ENV_MSVS
            FILETIME lpCreationTime, lpExitTime, lpKernelTime, lpUserTime;
            LARGE_INTEGER start, finish;

            GetProcessTimes(GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime);
            start.LowPart = lpUserTime.dwLowDateTime;
            start.HighPart = lpUserTime.dwHighDateTime;

            // We have 100-nanoseconds time unit. Convert them to seconds according to scale
            start.QuadPart *= (scale * 0.0000001);
#else
            int result;

            // Function returns rusage structure which contains timeval structure 
            // with seconds and microseconds values
            //rusage rusage_start, rusage_finish;
            //result = getrusage(RUSAGE_SELF, &rusage_start);
            //if (result != 0) return 0;

            // Returns seconds and nanoseconds values
            timespec ts_start, ts_finish;
            result = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts_start);
            if (result != 0) return 0;
#endif

            func(std::forward<Args>(args)...);

#ifdef ENV_MSVS

            GetProcessTimes(GetCurrentProcess(), &lpCreationTime, &lpExitTime, &lpKernelTime, &lpUserTime);
            finish.LowPart = lpUserTime.dwLowDateTime;
            finish.HighPart = lpUserTime.dwHighDateTime;

            // We have 100-nanoseconds time unit. Convert them to seconds
            finish.QuadPart *= (scale * 0.0000001);

            return static_cast<typename Period::rep>(finish.QuadPart - start.QuadPart);
#else
            result = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts_finish);

            // Convert according to scale 
            double start = scale * (ts_start.tv_sec + static_cast<double>(ts_start.tv_nsec) * 0.000000001);
            double finish = scale * (ts_finish.tv_sec + static_cast<double>(ts_finish.tv_nsec) * 0.000000001);

            return static_cast<typename Period::rep>(finish - start);

            //result = getrusage(RUSAGE_SELF, &rusage_finish);
            //if (result != 0) return 0;

            //double start_user_time = scale * (rusage_start.ru_utime.tv_sec + 
            //    static_cast<double>(rusage_start.ru_utime.tv_usec) * 0.000001);
            
            //double finish_user_time = scale * (rusage_finish.ru_utime.tv_sec +
            //    static_cast<double>(rusage_finish.ru_utime.tv_usec) * 0.000001);

            //double start_sys_time = scale * (rusage_start.ru_stime.tv_sec + 
            //    static_cast<double>(rusage_start.ru_stime.tv_usec) * 0.000001);
            
            //double finish_sys_time = scale * (rusage_finish.ru_stime.tv_sec +
            //    static_cast<double>(rusage_finish.ru_stime.tv_usec) * 0.000001);

            //return static_cast<typename Period::rep>(
            //       (finish_user_time + finish_sys_time) - (start_user_time + start_sys_time));
#endif
        }

        /*  Measure CPU ticks natively on Windows using "__rdtscp" intrinsic 
            This method should be used only to measure small functions ( < 2 sec execution)

            Notes: no call to CPUID as starting from Pentium processors with MMX support

        */

        template <class F, class... Args>
        static auto measure_cpu_ticks(F func, Args&&... args) -> unsigned long long
        {
            unsigned long long start{}, finish{};

#if defined (ENV_MSVS)

            // Support for the __rdtscp: [EAX=80000001h; EDX:27]
            int CPUInfo[4];
            __cpuid(CPUInfo, 0x80000001);
            bool is_rdtscp_avail = CPUInfo[3] & (1 << 27);

            // Is invariant TSC available? (modern CPUs)
            __cpuid(CPUInfo, 0x80000007);
            bool is_invar_tsc_avail = CPUInfo[3] & (1 << 8);

            // Support for the __rdtsc: [EAX=1h; EDX:4]
            __cpuid(CPUInfo, 1);
            bool is_rdtsc_avail = CPUInfo[3] & (1 << 4);

            if (!is_rdtsc_avail) return 0;

            auto fn_rdts = [](bool is_rdtscp_avail) -> unsigned long long int {
                if (is_rdtscp_avail)
                {
                    // Use serialized intrinsic if we can
                    unsigned int tsc_aux;
                    return __rdtscp(&tsc_aux);
                }

#if defined (ENV_x86)
                // Shows value that is twice as large as when simply invoke __rdtsc() ?
                unsigned int lo, hi;

                // TODO: compute the CPUID overhead 

                __asm {
                    lfence
                    rdtsc
                    mov lo, eax
                    mov hi, edx
                }

                return ((uint64_t)hi << 32) | lo;

#elif defined (ENV_x64)
                return __rdtsc();
#else
                return 0;
#endif
            };

            start = fn_rdts(is_rdtscp_avail);

            func(std::forward<Args>(args)...);

            finish = fn_rdts(is_rdtscp_avail);

#else
            // Linux

            auto fn_rdts = []() -> unsigned long long int {

#if defined (ENV_x86)

                unsigned long long int x;
                __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
                return x;

#elif defined (ENV_x64)

                unsigned int lo,hi;
                __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
                return ((uint64_t)hi << 32) | lo;
            
#else
                return 0;
#endif
            };


            start = fn_rdts();

            func(std::forward<Args>(args)...);

            finish = fn_rdts();

#endif            
            return finish - start;
        }
    };

    /* Continuous measurements: each measurement defines how much time has been passed since pinpoint */
    template <class Period = nanoseconds<long long>, bool MeasureCPU = false>
    class stopwatch_timer final
    {
    public:
        
        stopwatch_timer() { schedule(); }
        
        stopwatch_timer(const stopwatch_timer& other) = delete;
        stopwatch_timer& operator=(const stopwatch_timer& other) = delete;

        auto schedule() -> void {
            ts_wall_time.clear();
            pinpoint = std::chrono::high_resolution_clock::now();
        }

        auto snapshot() -> void {
            auto duration = std::chrono::duration_cast<Period>(
                std::chrono::high_resolution_clock::now() - pinpoint);

            ts_wall_time.push_back(duration.count());
        }

        auto get_wall() -> std::vector < typename Period::rep > {
            return ts_wall_time;
        }

        template <int N>
        auto get_wall() -> typename Period::rep
        {
            if (ts_wall_time.size() == 0) return 0;
            return N < ts_wall_time.size() ? ts_wall_time[N] : ts_wall_time[ts_wall_time.size() - 1];
        }

    private:
        std::chrono::high_resolution_clock::time_point pinpoint;
        std::vector<typename Period::rep> ts_wall_time, ts_cpu_time;
    };

    /* Continuous measurements: each measurement defines 
     * how much time has been passed between now and the last measurement
     */
    template <class Period = nanoseconds<long long>>
    class duration_timer final
    {
    public:

        duration_timer() { schedule(); }

        duration_timer(const duration_timer& other) = delete;
        duration_timer& operator=(const duration_timer& other) = delete;

        auto schedule() -> void {
            pp_prev = std::chrono::high_resolution_clock::now();
            pp_next = pp_prev;
            timestamps.clear();
        }

        auto snapshot() -> void {
            auto temp = std::chrono::high_resolution_clock::now();
            pp_prev = pp_next;
            auto duration = std::chrono::duration_cast<Period>(temp - pp_prev);

            timestamps.push_back(duration.count());

            pp_next = std::chrono::high_resolution_clock::now();
        }

        auto get() -> std::vector < typename Period::rep > {
            return timestamps;
        }

        template <int N>
        auto get() -> typename Period::rep {
            return N < timestamps.size() ? timestamps[N] : timestamps[timestamps.size() - 1];
        }

        auto median() -> double {
            if (timestamps.size() == 0) return 0;
            if (timestamps.size() == 1) return timestamps[0];
            
            size_t idx = timestamps.size() / 2;

            // Copy vector
            std::vector<typename Period::rep> tmp = timestamps;

            // Sort vector
            std::sort(begin(tmp), end(tmp));
            
            // Get median
            if (timestamps.size() % 2 == 0)
                return (tmp[idx] + tmp[idx + 1]) / 2;
            else
                return tmp[idx];
        }

        auto mean() -> double {
            if (timestamps.size() == 0) return 0;
            if (timestamps.size() == 1) return timestamps[0];

            return std::accumulate(begin(timestamps), end(timestamps), 0) / timestamps.size();
        }

    private:
        std::chrono::high_resolution_clock::time_point pp_prev, pp_next;
        std::vector<typename Period::rep> timestamps;
    };
}
