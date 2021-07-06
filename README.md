# cpp-misc
Collection of interesting c++-related stuff (optimizations, lack of optimizations, benchmarks, etc.):
+ [Returning `bool` from predicate may prevent `std::count_if` auto-vectorization](https://github.com/niculaionut/cpp-misc/blob/main/bool_returned_prevents_vectorization.md);
+ [When anticipating auto-vectorization, beware of type mismatches](https://github.com/niculaionut/cpp-misc/blob/main/simd_prefers_32bit_data.md).

# Notes
+ For local benchmark outputs, if not otherwise specified, the CPU used is `Intel(R) Core(TM) i5-8265U`.
