## When anticipating auto-vectorization, beware of type mismatches

### Details

[Read previous related post](https://github.com/niculaionut/cpp-misc/blob/main/bool_returned_prevents_vectorization.md)

In the case of a vector consisting of 32-bit values, using a 32-bit integer instead of a 64-bit integer to store the count of values satysfying the predicate provides more efficient SIMD optimizations.

Suppose we want to count the number of even values in a vector consisting of `std::uint32_t` data.

### The ideal case:

Load 8 integer values in a YMM register, do an `and not` operation with a register filled with 8 `0x1` values, add the result to the count register.
```
i0|i1|i2|i3|i4|i5|i6|i7   op: AND NOT
 1| 1| 1| 1| 1| 1| 1| 1
-----------------------
j0|j1|j2|j3|j4|j5|j6|j7

r0...7 += j0...7            // add j0...7 to r0...7

________________
i0...7 - register storing current 8 integers being evaluated
j0...7 - register storing result of the AND NOT operation
r0...7 - register storing the final results for each column
```

The gcc assembly output reflects this:
```asm
.L4:
        vmovdqu ymm3, YMMWORD PTR [rax]    // load 8 integer values into i0...7 
        add     rax, 32
        vpandn  ymm0, ymm3, ymm2           // j0...7 := ( i0...7 AND NOT 1|1|1|1|1|1|1|1 )
        vpaddd  ymm1, ymm1, ymm0           // r0...7 += j0...7
        cmp     rax, rdx
        jne     .L4
```

### The non-ideal case:

Say that the data in the vector consists of 32-bit integers, but the counter variable is a 64-bit integer. The previous operations won't work because even though `i0...7` and `j0...7` remain the same, `r` will only be able to hold 4 values of 64-bits. The operations roughly have the following idea:
```
i0|i1|i2|i3|i4|i5|i6|i7   op: AND NOT
 1| 1| 1| 1| 1| 1| 1| 1
-----------------------
j0|j1|j2|j3|j4|j5|j6|j7

// extract j0...3 and j4...7 into 2 YMM registers and zero-extend the values
t0...3 := j0...3
u0...3 := j4...7

t0...3 += u0...3            // add u0...3 to t0...3

r0...3 += t0...3            // add t0...3 to r0...3

________________
i0...7 - register storing current 8 integers being evaluated
j0...7 - register storing result of the AND NOT operation
t0...3 - register initially storing the zero-extended values j0...3
u0...3 - register initially storing the zero-extended values j4...7
r0...7 - register storing the final results for each column
```

The gcc assembly output for this case is as follows:
```asm
.L4:
        vmovdqu ymm4, YMMWORD PTR [rax]
        add     rax, 32
        vpandn  ymm0, ymm4, ymm3
        vpmovzxdq       ymm1, xmm0
        vextracti128    xmm0, ymm0, 0x1
        vpmovzxdq       ymm0, xmm0
        vpaddq  ymm0, ymm1, ymm0
        vpaddq  ymm2, ymm2, ymm0
        cmp     rax, rdx
        jne     .L4
```

### Don't do more work than you have to

The `libstdc++` and `libc++` implementations of `std::count_if` don't provide such granularity for the type of the variable that stores the count. They default to the `difference_type` of the iterator, which on x86_64 platforms is typically `long` (8 bytes). The implementations are as follows:

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

### Solving the type mismatch

[Benchmark source file](https://github.com/niculaionut/cpp-misc/blob/main/simd_prefers_32bit_data.bench.cpp)\
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
+ Important thing to keep in mind when micro-benchmarking very tight loops: [Code alignment issues](https://easyperf.net/blog/2018/01/18/Code_alignment_issues);
+ Tested on gcc 10.2 and clang 11.0;
+ As observed in the benchmark output, speed improvement is higher for smaller vectors (e.g. ~2x improvement for `2^14` elements). Clang achieves this by default while GCC needs the `-funroll-loops` flag (or manual directives/function attributes), otherwise it stagnates at ~1.5x improvement;
+ See `libbenchmark`'s [compare.py](https://github.com/google/benchmark/blob/main/docs/tools.md) for details regarding the output format;
+ Benchmark from above was compiled with:
```sh
Cclangbench() {
    clang++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -fno-rtti -flto "$@" -lbenchmark
}
```
