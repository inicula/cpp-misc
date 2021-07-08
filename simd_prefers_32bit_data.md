## When anticipating auto-vectorization, beware of type mismatches

### Details

[Read previous related post](https://github.com/niculaionut/cpp-misc/blob/main/bool_returned_prevents_vectorization.md)

In the case of a vector consisting of 32-bit values, using a 32-bit integer instead of a 64-bit integer to store the count of values satysfying the predicate provides more efficient SIMD optimizations.

Suppose we want to count the number of even values in a vector of arbitrary size, consisting of `std::uint32_t` data, while having at our disposal the 256-bit `YMM` registers. Also, assume that a `std::uint32_t` variable is sufficient for storing the number of even values in the vector.

### The ideal case for the hot loop:

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

The gcc assembly output reflects this (compiled with `-O3`, without loop unrolling):
```asm
.L4:
        vmovdqu ymm3, YMMWORD PTR [rax]    // load 8 integer values into i0...7 
        add     rax, 32
        vpandn  ymm0, ymm3, ymm2           // j0...7 := ( i0...7 AND NOT 1|1|1|1|1|1|1|1 )
        vpaddd  ymm1, ymm1, ymm0           // r0...7 += j0...7
        cmp     rax, rdx
        jne     .L4
```

### The non-ideal case for the hot loop:

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
[ionut@wtk:~/repos/cpp-misc]$ compare.py filters ./a.out assume_difference_type assume_element_type --benchmark_counters_tabular=true 2>/dev/null

RUNNING: ./a.out --benchmark_counters_tabular=true --benchmark_filter=assume_difference_type --benchmark_out=/tmp/tmpkndhdcnk
-------------------------------------------------------------------------------------------
Benchmark                                Time             CPU   Iterations bytes_per_second
-------------------------------------------------------------------------------------------
assume_difference_type/1024           92.0 ns         92.0 ns      7749175       41.4612G/s
assume_difference_type/4096            371 ns          371 ns      1877553       41.0957G/s
assume_difference_type/16384          1517 ns         1517 ns       463339       40.2247G/s
assume_difference_type/65536          6662 ns         6662 ns       104313       36.6448G/s
assume_difference_type/262144        29445 ns        29444 ns        23765       33.1666G/s
assume_difference_type/1048576      118469 ns       118468 ns         5809       32.9729G/s
assume_difference_type/4194304     1037006 ns      1036980 ns          645       15.0678G/s
assume_difference_type/16777216    4474535 ns      4474527 ns          154        13.968G/s
assume_difference_type/33554432    9195245 ns      9195106 ns           75       13.5942G/s
RUNNING: ./a.out --benchmark_counters_tabular=true --benchmark_filter=assume_element_type --benchmark_out=/tmp/tmpin94gfuh
----------------------------------------------------------------------------------------
Benchmark                             Time             CPU   Iterations bytes_per_second
----------------------------------------------------------------------------------------
assume_element_type/1024           39.5 ns         39.5 ns     17768812       96.4824G/s
assume_element_type/4096            146 ns          146 ns      4780845       104.412G/s
assume_element_type/16384           713 ns          713 ns       988855       85.6488G/s
assume_element_type/65536          3419 ns         3419 ns       206945       71.4099G/s
assume_element_type/262144        17225 ns        17199 ns        40472       56.7796G/s
assume_element_type/1048576       73137 ns        73022 ns        10269       53.4941G/s
assume_element_type/4194304      898927 ns       898926 ns          842       17.3819G/s
assume_element_type/16777216    4090342 ns      4090105 ns          170       15.2808G/s
assume_element_type/33554432    8357242 ns      8357298 ns           82        14.957G/s
Comparing assume_difference_type to assume_element_type (from ./a.out)
Benchmark                                                                   Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------------------
[assume_difference_type vs. assume_element_type]/1024                    -0.5703         -0.5703            92            40            92            40
[assume_difference_type vs. assume_element_type]/4096                    -0.6064         -0.6064           371           146           371           146
[assume_difference_type vs. assume_element_type]/16384                   -0.5304         -0.5304          1517           713          1517           713
[assume_difference_type vs. assume_element_type]/65536                   -0.4868         -0.4868          6662          3419          6662          3419
[assume_difference_type vs. assume_element_type]/262144                  -0.4150         -0.4159         29445         17225         29444         17199
[assume_difference_type vs. assume_element_type]/1048576                 -0.3826         -0.3836        118469         73137        118468         73022
[assume_difference_type vs. assume_element_type]/4194304                 -0.1332         -0.1331       1037006        898927       1036980        898926
[assume_difference_type vs. assume_element_type]/16777216                -0.0859         -0.0859       4474535       4090342       4474527       4090105
[assume_difference_type vs. assume_element_type]/33554432                -0.0911         -0.0911       9195245       8357242       9195106       8357298
```
\
If we're dealing with smaller data types for this task, the speed-up will be higher because the mismatched version will get worse. There will be more unpacking instructions needed to put 16-bit results (following the `and-not` operation) into 64-bit counters than there will be to put 32-bit results into 64-bit counters.

16-bit data, 64-bit counter vs. 16-bit data, 16-bit counter:
```
Benchmark                                                                   Time             CPU      Time Old      Time New       CPU Old       CPU New
--------------------------------------------------------------------------------------------------------------------------------------------------------
[assume_difference_type vs. assume_element_type]/1024                    -0.7788         -0.7788            86            19            86            19
[assume_difference_type vs. assume_element_type]/4096                    -0.7617         -0.7617           319            76           319            76
[assume_difference_type vs. assume_element_type]/16384                   -0.7673         -0.7673          1260           293          1260           293
[assume_difference_type vs. assume_element_type]/65536                   -0.7013         -0.7013          5252          1568          5252          1568
[assume_difference_type vs. assume_element_type]/262144                  -0.6470         -0.6470         23372          8250         23372          8249
[assume_difference_type vs. assume_element_type]/1048576                 -0.6248         -0.6244         92841         34838         92766         34838
[assume_difference_type vs. assume_element_type]/4194304                 -0.4449         -0.4449        559309        310459        559262        310461
[assume_difference_type vs. assume_element_type]/16777216                -0.2164         -0.2164       2466129       1932422       2465997       1932392
[assume_difference_type vs. assume_element_type]/33554432                -0.1585         -0.1584       4986463       4196321       4986291       4196299
```
8-bit data, 64-bit counter vs. 8-bit data, 8-bit counter:
```
Benchmark                                                                    Time             CPU      Time Old      Time New       CPU Old       CPU New
---------------------------------------------------------------------------------------------------------------------------------------------------------
[assume_difference_type vs. assume_element_type]/1024                     -0.8855         -0.8855            90            10            90            10
[assume_difference_type vs. assume_element_type]/4096                     -0.8959         -0.8959           359            37           359            37
[assume_difference_type vs. assume_element_type]/32768                    -0.8961         -0.8960          2843           296          2843           296
[assume_difference_type vs. assume_element_type]/262144                   -0.8492         -0.8491         23140          3491         23139          3491
[assume_difference_type vs. assume_element_type]/2097152                  -0.8169         -0.8169        187886         34403        187879         34403
[assume_difference_type vs. assume_element_type]/16777216                 -0.5038         -0.5038       1779460        882999       1779407        882952
[assume_difference_type vs. assume_element_type]/134217728                -0.4224         -0.4224      14438934       8340119      14438215       8340170
[assume_difference_type vs. assume_element_type]/268435456                -0.4234         -0.4233      29039393      16745090      29038266      16745005
```

### Notes
+ Important thing to keep in mind when micro-benchmarking very tight loops: [Code alignment issues](https://easyperf.net/blog/2018/01/18/Code_alignment_issues);
+ Tested on gcc 10.2 and clang 11.0;
+ As observed in the benchmark output, speed improvement is higher for smaller vectors (e.g. ~2x improvement for `2^14` elements). Clang achieves this with its more aggressive unrolling when compiled with `-O3`, while GCC additionally needs the `-funroll-loops` flag (or manual directives/function attributes), otherwise it stagnates at ~1.5x improvement;
+ See `libbenchmark`'s [compare.py](https://github.com/google/benchmark/blob/main/docs/tools.md) for details regarding the output format;
+ Benchmark from above was compiled with:
```sh
Cclangbench() {
    clang++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -fno-rtti -flto "$@" -lbenchmark
}
```
