### Details

[Read previous related post](https://github.com/niculaionut/cpp-misc/blob/main/bool_returned_prevents_vectorization.md)

More efficient SIMD optimizations when using 32-bit integer instead of 64-bit integer to store the count. Significant speed-ups on both gcc and clang.

The `libstdc++` and `libc++` implementations of `std::count_if` don't provide such granularity for the type of the variable that stores the count. They default to the `difference_type` of the iterator, which on x86_64 platforms is typically `long`. The implementations are as follows:

```cpp
template<class _InputIterator, class _Predicate>
_LIBCPP_NODISCARD_EXT inline _LIBCPP_INLINE_VISIBILITY _LIBCPP_CONSTEXPR_AFTER_CXX17
    typename iterator_traits<_InputIterator>::difference_type
    count_if(_InputIterator __first, _InputIterator __last, _Predicate __pred)
{
        typename iterator_traits<_InputIterator>::difference_type __r(0);
        for(; __first != __last; ++__first)
                if(__pred(*__first))
                        ++__r;
        return __r;
}
```

```cpp
template<typename _InputIterator, typename _Predicate>
_GLIBCXX20_CONSTEXPR typename iterator_traits<_InputIterator>::difference_type
__count_if(_InputIterator __first, _InputIterator __last, _Predicate __pred)
{
        typename iterator_traits<_InputIterator>::difference_type __n = 0;
        for(; __first != __last; ++__first)
                if(__pred(__first))
                        ++__n;
        return __n;
}
```
\
[Benchmark source file (counting number of even values in a vector)](https://github.com/niculaionut/cpp-misc/blob/main/simd_prefers_32bit_data.bench.cpp)\
Benchmark output:
```
[ionut@wtk:~/repos/cpp-misc]$ compare.py filters './a.out' assume_difference_type assume_unsigned --benchmark_counters_tabular=true 2>/dev/null

RUNNING: ./a.out --benchmark_counters_tabular=true --benchmark_filter=assume_difference_type --benchmark_out=/tmp/tmpkw9yuc_c
-------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations bytes_per_second
-------------------------------------------------------------------------------------------
assume_difference_type/1024           87.5 ns         87.5 ns      8144265       43.6106G/s
assume_difference_type/4096            321 ns          321 ns      2174887       47.5026G/s
assume_difference_type/16384          1397 ns         1397 ns       503347       43.6992G/s
assume_difference_type/65536          6030 ns         6030 ns       114994       40.4868G/s
assume_difference_type/262144        27919 ns        27919 ns        25124       34.9779G/s
assume_difference_type/1048576      111319 ns       111320 ns         6156       35.0903G/s
assume_difference_type/4194304     1022775 ns      1022775 ns          647       15.2771G/s
assume_difference_type/16777216    4480582 ns      4480578 ns          155       13.9491G/s
assume_difference_type/33554432    9120312 ns      9120252 ns           75       13.7058G/s
RUNNING: ./a.out --benchmark_counters_tabular=true --benchmark_filter=assume_unsigned --benchmark_out=/tmp/tmpele8mouh
------------------------------------------------------------------------------------
Benchmark                         Time             CPU   Iterations bytes_per_second
------------------------------------------------------------------------------------
assume_unsigned/1024           39.2 ns         39.2 ns     17703718       97.3024G/s
assume_unsigned/4096            146 ns          146 ns      4784824        104.16G/s
assume_unsigned/16384           707 ns          707 ns       992073       86.3406G/s
assume_unsigned/65536          3354 ns         3354 ns       210175       72.7952G/s
assume_unsigned/262144        19086 ns        19086 ns        37195       51.1671G/s
assume_unsigned/1048576       76389 ns        76387 ns         8702       51.1379G/s
assume_unsigned/4194304      860988 ns       860958 ns          735       18.1484G/s
assume_unsigned/16777216    4088298 ns      4088282 ns          169       15.2876G/s
assume_unsigned/33554432    8477987 ns      8477803 ns           82       14.7444G/s
Comparing assume_difference_type to assume_unsigned (from ./a.out)
Benchmark                                                               Time             CPU      Time Old      Time New       CPU Old       CPU New
----------------------------------------------------------------------------------------------------------------------------------------------------
[assume_difference_type vs. assume_unsigned]/1024                    -0.5518         -0.5518            87            39            87            39
[assume_difference_type vs. assume_unsigned]/4096                    -0.5440         -0.5439           321           146           321           146
[assume_difference_type vs. assume_unsigned]/16384                   -0.4939         -0.4939          1397           707          1397           707
[assume_difference_type vs. assume_unsigned]/65536                   -0.4438         -0.4438          6030          3354          6030          3354
[assume_difference_type vs. assume_unsigned]/262144                  -0.3164         -0.3164         27919         19086         27919         19086
[assume_difference_type vs. assume_unsigned]/1048576                 -0.3138         -0.3138        111319         76389        111320         76387
[assume_difference_type vs. assume_unsigned]/4194304                 -0.1582         -0.1582       1022775        860988       1022775        860958
[assume_difference_type vs. assume_unsigned]/16777216                -0.0876         -0.0876       4480582       4088298       4480578       4088282
[assume_difference_type vs. assume_unsigned]/33554432                -0.0704         -0.0704       9120312       8477987       9120252       8477803
```

### Notes
+ Tested on gcc 10.2 and clang 11.0;
+ As observed in the benchmark output, speed improvement is higher for smaller vectors (e.g. ~2x improvement for `2^14` elements). Clang achieves this by default while GCC needs the `-funroll-loops` flag (or manual directives/function attributes), otherwise it stagnates at ~1.5x improvement;
+ See `libbenchmark`'s [compare.py](https://github.com/google/benchmark/blob/main/docs/tools.md) for details regarding the output format.
+ Benchmark from above was compiled with:
```sh
Cclangbench() {
    clang++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -fno-rtti -flto "$@" -lbenchmark
}
```
