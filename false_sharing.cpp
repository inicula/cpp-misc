#include <iostream>
#include <cstdint>
#include <thread>

/* GNU specific */
#define DO_NOT_OPTIMIZE __attribute__((optimize("O0")))

/* Try with: `alignas(64)` */
struct alignas(8) Storage
{
        std::uint64_t val{0};
};

static Storage results[4] = {};

static void DO_NOT_OPTIMIZE
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
        std::thread t2(do_stuff, 2);
        std::thread t3(do_stuff, 3);

        t0.join();
        t1.join();
        t2.join();
        t3.join();

        std::cout << results[0].val + results[1].val + results[2].val + results[3].val << '\n';
}
