#include <benchmark/benchmark.h>
#include <random>
#include <ctime>
#include <vector>

using VecIter = std::vector<unsigned>::iterator;
using VecIterDiff = std::iterator_traits<VecIter>::difference_type;

static constexpr std::size_t SIZE = 1 << 20;

static const auto test_vec = []()
{
        std::vector<unsigned> vec;
        vec.reserve(SIZE);

        std::srand(std::time(0));
        for(std::size_t i = 0; i < SIZE; ++i)
        {
                vec.push_back(std::rand());
        }

        return vec;
}();

static constexpr auto is_even = []<typename T>(const unsigned el) -> T
{
        return el % 2 == 0;
};

template<typename ResType, typename It>
auto mcount_if(It first, const It last, auto pred)
{
        ResType result = 0;
        for(; first != last; ++first)
        {
                result += pred.template operator()<ResType>(*first);
        }
        return result;
}

static void assume_difference_type(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp =
                    mcount_if<VecIterDiff>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(assume_difference_type)->Unit(benchmark::kMicrosecond);

static void assume_unsigned(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp =
                    mcount_if<unsigned>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(assume_unsigned)->Unit(benchmark::kMicrosecond);

static void std_countif(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp = std::count_if(test_vec.begin(), test_vec.end(),
                                               [](const unsigned el)
                                               {
                                                       return el % 2 == 0;
                                               });
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(std_countif)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
