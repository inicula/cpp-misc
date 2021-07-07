## Returning `bool` from predicate may prevent `std::count_if` auto-vectorization

### Details

GCC doesn't vectorize the ```libstdc++``` implementation of ```std::count_if``` in the case of a simple counting of even integers in a vector. Modyfing certain implementation details provides a 3-4x gain in speed.
The ```libstdc++``` implementation is as follows:

```cpp
template<typename _InputIterator, typename _Predicate>
_GLIBCXX20_CONSTEXPR inline typename iterator_traits<_InputIterator>::difference_type
count_if(_InputIterator __first, _InputIterator __last, _Predicate __pred)
{
        // concept requirements
        __glibcxx_function_requires(_InputIteratorConcept<_InputIterator>)
            __glibcxx_function_requires(
                _UnaryPredicateConcept<_Predicate,
                                       typename iterator_traits<_InputIterator>::value_type>)
                __glibcxx_requires_valid_range(__first, __last);

        return std::__count_if(__first, __last, __gnu_cxx::__ops::__pred_iter(__pred));
}

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

Before calling the main ```count_if``` function, the predicate is wrapped in a ```_Iter_pred``` struct: ```__gnu_cxx::__ops::__pred_iter(__pred)```.
```_Iter_pred``` wraps a predicate in a callable object that takes an iterator type (as opposed to a vector element type). The call operator completely ignores the type returned by the underlying predicate and always returns ```bool```:

```cpp
template<typename _Predicate>
struct _Iter_pred
{
        _Predicate _M_pred;

        _GLIBCXX20_CONSTEXPR
        explicit _Iter_pred(_Predicate __pred)
            : _M_pred(_GLIBCXX_MOVE(__pred))
        {
        }

        template<typename _Iterator>
        _GLIBCXX20_CONSTEXPR bool operator()(_Iterator __it)
        {
                return bool(_M_pred(*__it));
        }
};
```

As observed in the [.godbolt.cpp](bool_returned_prevents_vectorization.godbolt.cpp) and [.bench.cpp](bool_returned_prevents_vectorization.bench.cpp) files, GCC vectorizes the code successfully when the predicate wrapper doesn't return ```bool```. Upon modifying the ```_Iter_pred``` implementation of ```operator()``` to return ```auto``` (i.e. the return type of the underlying predicate) instead of a ```bool``` value, GCC applies the vectorizations.

[Benchmark](bool_returned_prevents_vectorization.bench.cpp) output:
```
[ionut@wtk:~/repos/cpp-misc]$ ./a.out 2>/dev/null

--------------------------------------------------------
Benchmark              Time             CPU   Iterations
--------------------------------------------------------
v_return_bool        630 us          630 us         1121
v_return_auto        185 us          185 us         3575
std_count_if         609 us          609 us         1109
```

### Notes:
+ Tested on gcc version 11.2 / 10.2;
+ Clang successfully vectorizes the code even if the predicate (or the predicate wrapper) returns ```bool```;
+ Integrating (boolean) variables as arithmetic (i.e. doing `result += pred(first)` instead of `if(pred(iter)) { ++result; }` may reduce branching, help the compiler make more optimizations and thus further improve the speed. When compiled with GCC, `v_return_auto` gets a ~1.2x improvement if such a modification is applied to the `mcount_if` function. Alexandrescu shows some potential benefits of this approach in his [2019 CppCon talk](https://youtu.be/FJJTYQYB1JQ);
+ For more auto-vectorization details regarding `std::count_if`, see [When anticipating auto-vectorization, beware of type mismatches](https://github.com/niculaionut/cpp-misc/blob/main/simd_prefers_32bit_data.md);
+ Benchmark source file was compiled with command:
```sh
Cbench() {
    g++ -std=c++20 -Wall -Wextra -Wpedantic -O3 -march=native -fno-exceptions -flto "$@" -lbenchmark
}
```
