#include <benchmark/benchmark.h>
#include <random>
#include <ctime>
#include <vector>

const std::size_t SIZE = 1 << 20;

const auto test_vec = []()
{
        std::vector<int> vec;
        vec.reserve(SIZE);

        std::srand(std::time(0));
        for(std::size_t i = 0; i < SIZE; ++i)
        {
                vec.push_back(std::rand());
        }

        return vec;
}();

template<typename _Predicate>
struct _Iter_pred_auto
{
        _Predicate _M_pred;

        _GLIBCXX20_CONSTEXPR
        explicit _Iter_pred_auto(_Predicate __pred)
            : _M_pred(_GLIBCXX_MOVE(__pred))
        {
        }

        // RETURNS THE ORIGINAL TYPE OF THE LAMBDA
        template<typename _Iterator>
        _GLIBCXX20_CONSTEXPR auto operator()(_Iterator __it) const
        {
                return _M_pred(*__it);
        }
};

template<typename _Predicate>
struct _Iter_pred_bool
{
        _Predicate _M_pred;

        _GLIBCXX20_CONSTEXPR
        explicit _Iter_pred_bool(_Predicate __pred)
            : _M_pred(_GLIBCXX_MOVE(__pred))
        {
        }

        // IGNORES THE ORIGINAL TYPE RETURNED BY THE LAMBDA
        template<typename _Iterator>
        _GLIBCXX20_CONSTEXPR bool operator()(_Iterator __it) const
        {
                return bool(_M_pred(*__it));
        }
};

const auto is_even_int = [](const int el) -> int
{
        return el % 2 == 0;
};

template<typename It>
auto mcount_if(It first, const It last, const auto pred)
{
        typename std::iterator_traits<It>::difference_type result = 0;
        for(; first != last; ++first)
        {
                if(pred(first))
                {
                        ++result;
                }
        }
        return result;
}

auto version1(const std::vector<int>& vec)
{
        return mcount_if(vec.begin(), vec.end(), _Iter_pred_bool{is_even_int});
}

auto version2(const std::vector<int>& vec)
{
        return mcount_if(vec.begin(), vec.end(), _Iter_pred_auto{is_even_int});
}

auto version3(const std::vector<int>& vec)
{
        return std::count_if(vec.begin(), vec.end(), is_even_int);
}

static void V1(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp = version1(test_vec);
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(V1)->Unit(benchmark::kMicrosecond);

static void V2(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp = version2(test_vec);
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(V2)->Unit(benchmark::kMicrosecond);

static void V3(benchmark::State& state)
{
        for(auto _ : state)
        {
                const auto tmp = version3(test_vec);
                benchmark::DoNotOptimize(tmp);
        }
}
BENCHMARK(V3)->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
