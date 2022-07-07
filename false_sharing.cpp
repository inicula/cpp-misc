#include <iostream>
#include <thread>

using u64 = std::uint64_t;

/* GNU specific */
#define DO_NOT_OPTIMIZE __attribute__((optimize("O0")))

/* Try with: `alignas(8)` (minimum alignment for std::uint64_t) */
struct alignas(64) Storage
{
        u64 val{0};
};

static Storage results[2] = {};

void DO_NOT_OPTIMIZE
do_stuff(const std::size_t idx)
{
        auto& ref = results[idx];

        for(std::size_t i = 0; i < 1 << 26; ++i)
        {
                ++ref.val;
        }
}

int
main()
{
        std::thread t0(do_stuff, 0);
        std::thread t1(do_stuff, 1);

        t0.join();
        t1.join();

        std::cout << results[0].val + results[0].val << '\n';
}
