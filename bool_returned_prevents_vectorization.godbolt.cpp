// compile with -O3 -std=c++20

#include <vector>
#include <algorithm>

const auto is_even_bool = [](const int el)
{
    return el % 2 == 0;
};

const auto is_even_int = [](const int el) -> int
{
    return el % 2 == 0;
};

template<typename It>
auto mcount_if(It first, const It last, const auto pred)
{
    typename std::iterator_traits<It>::difference_type result = 0;
    while(first != last)
    {
        result += pred(*first++);
    }
    return result;
}

auto version1(const std::vector<int>& vec)
{
    return mcount_if(vec.begin(), vec.end(), is_even_bool);
}

auto version2(const std::vector<int>& vec)
{
    return mcount_if(vec.begin(), vec.end(), is_even_int);
}

auto version3(const std::vector<int>& vec)
{
    return std::count_if(vec.begin(), vec.end(), is_even_int);
}
