On top of the previously discussed optimizations, further improvements can be made if the data has more constraints, such as:
+ The vector's memory block containing the integers is aligned to 32 bytes (necessary for aligned loads into `ymm` registers);
+ The size of the vector is a multiple of the number of values evaluated per iteration in the hot loop. In the non-optimized version there will be branches that deal with the case where the vector hasn't been fully processed yet, but there aren't enough remaining integers in order to read a full `YMMWORD` from memory.

These constraints can be applied as follows:
```cpp
/* .cpp */
#include <cstdint>
#include <memory>

template<typename T>
consteval std::size_t values_per_ymmword()
{
        return 32 / sizeof(T);
}

template<bool MORE_OPTIMIZATIONS, typename T>
T count_even(const T* data, const std::size_t size)
{
        if constexpr(MORE_OPTIMIZATIONS)
        {
                /* Assume `size` is a multiple of the number of values that can
                 * fit in a YMMWORD (256 bits or 32 bytes). Additionally, assume
                 * that `size` is not 0. */
                if(size % values_per_ymmword<T>() != 0 || size == 0)
                {
                        __builtin_unreachable();
                }

                /* Assume `data` is 32-byte aligned  */
                data = std::assume_aligned<32>(data);
        }

        T result = 0;
        for(std::size_t i = 0; i != size; ++i)
        {
                result += (data[i] % 2 == 0);
        }
        return result;
}

using data_type = std::uint8_t;

/* instantiate both versions (the first one - with constraits, the second one - without constraints) */
template data_type count_even<true>(const data_type*, std::size_t);
template data_type count_even<false>(const data_type*, std::size_t);
```

Compiling on godbolt with GCC 11.0 (flags: `-std=c++20 -O3 -march=skylake-avx512 -fno-unroll-loops`), we get the following results:
```asm
; version with constraints

unsigned char count_even<true, unsigned char>(unsigned char const*, unsigned long):
        and     rsi, -32
        vpbroadcastb    ymm2, BYTE PTR .LC1[rip]
        add     rsi, rdi
        vpxor   xmm1, xmm1, xmm1
.L2:
        vpternlogd      ymm0, ymm0, YMMWORD PTR [rdi], 0x55
        add     rdi, 32
        vpand   ymm0, ymm0, ymm2
        vpaddb  ymm1, ymm1, ymm0
        cmp     rsi, rdi
        jne     .L2
        vextracti128    xmm0, ymm1, 0x1
        vpaddb  xmm0, xmm0, xmm1
        vpsrldq xmm1, xmm0, 8
        vpaddb  xmm0, xmm0, xmm1
        vpxor   xmm1, xmm1, xmm1
        vpsadbw xmm0, xmm0, xmm1
        vpextrb eax, xmm0, 0
        vzeroupper
        ret
```

```asm
; version without constraints

unsigned char count_even<false, unsigned char>(unsigned char const*, unsigned long):
        mov     rcx, rdi
        mov     rdx, rsi
        test    rsi, rsi
        je      .L13
        lea     rax, [rsi-1]
        cmp     rax, 30
        jbe     .L14
        and     rsi, -32
        vpbroadcastb    ymm2, BYTE PTR .LC1[rip]
        mov     rax, rdi
        add     rsi, rdi
        vpxor   xmm1, xmm1, xmm1
.L8:
        vmovdqu8        ymm3, YMMWORD PTR [rax]
        add     rax, 32
        vpandn  ymm0, ymm3, ymm2
        vpaddb  ymm1, ymm1, ymm0
        cmp     rax, rsi
        jne     .L8
        vextracti128    xmm0, ymm1, 0x1
        vpaddb  xmm0, xmm0, xmm1
        vpsrldq xmm1, xmm0, 8
        vpaddb  xmm0, xmm0, xmm1
        vpxor   xmm1, xmm1, xmm1
        vpsadbw xmm0, xmm0, xmm1
        mov     rax, rdx
        vpextrb r8d, xmm0, 0
        and     rax, -32
        test    dl, 31
        je      .L17
        vzeroupper
.L7:
        mov     rsi, rdx
        sub     rsi, rax
        lea     rdi, [rsi-1]
        cmp     rdi, 14
        jbe     .L11
        vmovdqu8        xmm4, XMMWORD PTR [rcx+rax]
        vpbroadcastb    xmm0, BYTE PTR .LC1[rip]
        vpandn  xmm0, xmm4, xmm0
        vpsrldq xmm1, xmm0, 8
        vpaddb  xmm0, xmm0, xmm1
        vpxor   xmm1, xmm1, xmm1
        vpsadbw xmm0, xmm0, xmm1
        vpextrb edi, xmm0, 0
        add     r8d, edi
        mov     rdi, rsi
        and     rdi, -16
        add     rax, rdi
        cmp     rsi, rdi
        je      .L5
.L11:
        movzx   esi, BYTE PTR [rcx+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+1]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+1+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+2]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+2+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+3]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+3+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+4]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+4+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+5]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+5+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+6]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+6+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+7]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+7+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+8]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+8+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+9]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+9+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+10]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+10+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+11]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+11+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+12]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+12+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+13]
        cmp     rdx, rsi
        je      .L5
        movzx   esi, BYTE PTR [rcx+13+rax]
        not     esi
        and     esi, 1
        add     r8d, esi
        lea     rsi, [rax+14]
        cmp     rdx, rsi
        je      .L5
        movzx   eax, BYTE PTR [rcx+14+rax]
        not     eax
        and     eax, 1
        add     r8d, eax
.L5:
        mov     eax, r8d
        ret
.L13:
        xor     r8d, r8d
        mov     eax, r8d
        ret
.L14:
        xor     eax, eax
        xor     r8d, r8d
        jmp     .L7
.L17:
        vzeroupper
        jmp     .L5
.LC1:
        .byte   1
```

#### Notes:
+ For such a simple operation, when the data is 32-byte aligned, the performance penalty for using an `unaligned load` compared to an `aligned load` instruction may be negligible.
+ The actual alignment of the data, however, may have a significant impact on performance: see [this post](https://www.agner.org/optimize/blog/read.php?i=415#423) on [Agner Fog](https://www.agner.org/)'s blog.
