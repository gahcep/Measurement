#include <iostream>
#include <iomanip>
#include "measurement.hpp"
#include <random>

using namespace measure;

// Functor
class Fib
{
public:

    unsigned int operator()(int N)
    {
        return fibonacchi(N);
    }

private:
    unsigned int fibonacchi(unsigned int N)
    {
        if (N == 1 || N == 0) return 1;

        return fibonacchi(N - 2) + fibonacchi(N - 1);
    }
};

// Function
unsigned int fibonacchi(unsigned int N)
{
    if (N == 1 || N == 0) return 1;

    return fibonacchi(N - 2) + fibonacchi(N - 1);
};

int main()
{
    std::vector<int, std::allocator<int>> vec;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(1, 10000000);

    vec.clear();

    // Synthetic load
    auto func = [&vec, &dist, &mt]() {
        for (int i = 0; i < 24000; ++i)
            vec.push_back(dist(mt));

        for (int i = 0; i < 10000; i++)
            vec.erase(begin(vec));

        for (int i = 0; i < 120000; ++i)
            vec.push_back(dist(mt));

        vec.clear();
        vec.reserve(0);
    };

    measure::timer<measure::nanoseconds<double>> timer;

    std::cout << "#### Simple Measurement Routine ####" << '\n';

    // C++ standard routines
    std::cout << "Measurement: C++ Standard" << '\n';
    std::cout << std::setprecision(15) << timer.measure_time_cplusplus(func) << " nanoseconds\n";

    // Wall time
    std::cout << "Measurement: Wall time" << '\n';
    std::cout << std::setprecision(15) << timer.measure_wall_time(func) << " nanoseconds\n";

    // CPU time
    std::cout << "Measurement: CPU time" << '\n';
    std::cout << std::setprecision(15) << timer.measure_cpu_time(func) << " nanoseconds\n";

    // CPU ticks
    std::cout << "Measurement: CPU ticks" << '\n';
    std::cout << std::setprecision(15) << timer.measure_cpu_ticks(func) << " nanoseconds\n";

    std::cout << '\n' << "#### Using Duration Timer ####" << '\n';

    measure::duration_timer<measure::milliseconds<double>> stimer;
    func();
    stimer.snapshot();
    func();
    stimer.snapshot();
    func();
    stimer.snapshot();
    std::cout << "3 Function Calls. Mean: " << stimer.mean() << " milliseconds\n";;

    std::cout << '\n' << "#### OpenMP Test ####" << '\n';

    auto fn_simp = [](){
        double sum{};
        for (long long i = 1; i < 100000000; i++){
            sum += log((double)i);
        }
    };

    auto fn_mp = [](){
        double sum{};
#pragma omp parallel for reduction(+ : sum)
        for (long long i = 1; i < 100000000; i++){
            sum += log((double)i);
        }
    };

    measure::timer<measure::milliseconds<double>> mtimer;

    // Wall time
    std::cout << "Measurement: Wall time: NO OpenMP" << '\n';
    std::cout << std::setprecision(15) << mtimer.measure_wall_time(fn_simp) << " milliseconds\n";

    std::cout << "Measurement: Wall time: OpenMP" << '\n';
    std::cout << std::setprecision(15) << mtimer.measure_wall_time(fn_mp) << " milliseconds\n";

    // CPU time
    std::cout << "Measurement: CPU time: NO OpenMP" << '\n';
    std::cout << std::setprecision(15) << mtimer.measure_cpu_time(fn_simp) << " milliseconds\n";

    std::cout << "Measurement: CPU time: OpenMP" << '\n';
    std::cout << std::setprecision(15) << mtimer.measure_cpu_time(fn_mp) << " milliseconds\n";

    std::cout << '\n' << "#### Using StopWatch Timer ####" << '\n';

    measure::stopwatch_timer<measure::milliseconds<double>> swtimer;
    fibonacchi(33);
    swtimer.snapshot();
    fibonacchi(33);
    swtimer.snapshot();
    fibonacchi(33);
    swtimer.snapshot();

    std::cout << "StopWatch: Stop #1: " << swtimer.get_wall<0>() << " milliseconds\n";
    std::cout << "StopWatch: Stop #2: " << swtimer.get_wall<1>() << " milliseconds\n";
    std::cout << "StopWatch: Stop #3: " << swtimer.get_wall<2>() << " milliseconds\n";
}