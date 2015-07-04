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

int main()
{
    std::vector<int, std::allocator<int>> vec1;

    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(1, 10000000);

    vec1.clear();

    auto func1 = [&vec1, &dist, &mt]() {
        for (int i = 0; i < 24000; ++i)
            vec1.push_back(dist(mt));

        for (int i = 0; i < 10000; i++)
            vec1.erase(begin(vec1));

        for (int i = 0; i < 120000; ++i)
            vec1.push_back(dist(mt));

        vec1.clear();
        vec1.reserve(0);
    };

    measure::timer<measure::nanoseconds<double>> timer;

    // C++ standard routines
    std::cout << "Measurement: C++ Standard" << '\n';
    std::cout << std::setprecision(15) << timer.measure_time_cplusplus(func1) << '\n';

    // Wall time
    std::cout << "Measurement: Wall time" << '\n';
    std::cout << std::setprecision(15) << timer.measure_wall_time(func1) << '\n';

    // CPU time
    std::cout << "Measurement: CPU time" << '\n';
    std::cout << std::setprecision(15) << timer.measure_cpu_time(func1) << '\n';

    // CPU ticks
    std::cout << "Measurement: CPU ticks" << '\n';
    std::cout << std::setprecision(15) << timer.measure_cpu_ticks(func1) << '\n';

    return 0; 
}
