### Details

[Read previous related post](https://github.com/niculaionut/cpp-misc/blob/main/bool_returned_prevents_vectorization.md)

More efficient SIMD optimizations when using 32-bit integer instead of 64-bit integer to store the count. 1.5x speed-up on both gcc and clang.

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
[Benchmark source file](https://github.com/niculaionut/cpp-misc/blob/main/simd_prefers_32bit_data.bench.cpp)\
Benchmark output:
```
2021-07-06T13:31:09+03:00
Running ./a.out
Run on (8 X 3900 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 256 KiB (x4)
  L3 Unified 6144 KiB (x1)
Load Average: 0.33, 0.28, 0.23
-----------------------------------------------------------------
Benchmark                       Time             CPU   Iterations
-----------------------------------------------------------------
assume_difference_type        128 us          128 us         5508
assume_unsigned              82.6 us         82.6 us         8240
std_countif                   131 us          131 us         5216
```
Compiled with:
```sh
Cclangbench() {
    clang++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -flto "$@" -lbenchmark
}
```

### Notes
+ Tested on gcc 10.2 and clang 11.0
