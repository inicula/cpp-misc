#include <benchmark/benchmark.h>
#include <random>
#include <ctime>
#include <vector>
#include <span>
#include <random>

using element_type = std::uint32_t;
using vec_iter_t = std::vector<element_type>::iterator;
using vec_iter_diff_t = std::iterator_traits<vec_iter_t>::difference_type;

static constexpr std::size_t SIZE = 1 << 25;

static const auto global_vec = []()
{
        std::vector<element_type> vec;
        vec.reserve(SIZE);

        std::mt19937 gen(std::random_device{}());
        std::uniform_int_distribution<element_type> distrib;
        for(std::size_t i = 0; i < SIZE; ++i)
        {
                vec.push_back(distrib(gen));
        }

        return vec;
}();

static constexpr auto is_even = []<typename T>(const element_type el) -> T
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
                    mcount_if<vec_iter_diff_t>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) *
                                sizeof(element_type));
}

static void assume_element_type(benchmark::State& state)
{
        const auto test_vec =
            std::span{global_vec.begin(), static_cast<std::size_t>(state.range(0))};

        for(auto _ : state)
        {
                const auto tmp =
                    mcount_if<element_type>(test_vec.begin(), test_vec.end(), is_even);
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) *
                                sizeof(element_type));
}

static void std_countif(benchmark::State& state)
{
        const auto test_vec =
            std::span{global_vec.begin(), static_cast<std::size_t>(state.range(0))};

        for(auto _ : state)
        {
                const auto tmp = std::count_if(test_vec.begin(), test_vec.end(),
                                               [](const element_type el)
                                               {
                                                       return el % 2 == 0;
                                               });
                benchmark::DoNotOptimize(tmp);
        }

        state.SetBytesProcessed(int64_t(state.iterations()) * int64_t(state.range(0)) *
                                sizeof(element_type));
}

static constexpr std::size_t STEP = 4ul;
static constexpr std::size_t LEFT = std::min(1ul << 10ul, SIZE);
static constexpr std::size_t RIGHT = std::min(1ul << 25ul, SIZE);

BENCHMARK(assume_difference_type)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);
BENCHMARK(assume_element_type)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);
BENCHMARK(std_countif)->RangeMultiplier(STEP)->Range(LEFT, RIGHT);

BENCHMARK_MAIN();
