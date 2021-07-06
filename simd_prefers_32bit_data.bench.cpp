#include <benchmark/benchmark.h>
#include <random>
#include <ctime>
#include <vector>
#include <span>

using VecIter = std::vector<unsigned>::iterator;
using VecIterDiff = std::iterator_traits<VecIter>::difference_type;

static constexpr std::size_t SIZE = 1 << 25;

static const auto global_vec = []()
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
        const auto test_vec =
            std::span{global_vec.begin(), static_cast<std::size_t>(state.range(0))};

        for(auto _ : state)
        {
                const auto tmp =
                    mcount_if<VecIterDiff>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) * 4);
}

static void assume_unsigned(benchmark::State& state)
{
        const auto test_vec =
            std::span{global_vec.begin(), static_cast<std::size_t>(state.range(0))};

        for(auto _ : state)
        {
                const auto tmp =
                    mcount_if<unsigned>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) * 4);
}

static void std_countif(benchmark::State& state)
{
        const auto test_vec =
            std::span{global_vec.begin(), static_cast<std::size_t>(state.range(0))};

        for(auto _ : state)
        {
                const auto tmp = std::count_if(test_vec.begin(), test_vec.end(),
                                               [](const unsigned el)
                                               {
                                                       return el % 2 == 0;
                                               });
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) * 4);
}

static constexpr std::size_t STEP = 4ul;
static constexpr std::size_t LEFT = std::min(1ul << 10ul, SIZE);
static constexpr std::size_t RIGHT = std::min(1ul << 25ul, SIZE);

BENCHMARK(assume_difference_type)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);
BENCHMARK(assume_unsigned)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);
BENCHMARK(std_countif)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);

BENCHMARK_MAIN();
