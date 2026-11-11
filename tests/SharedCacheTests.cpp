#include "Engine/Core/SharedCache.h"
#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <tbb/parallel_for.h>
#include <vector>

using namespace enzo;

TEST_CASE("A shared cache builds its value once across reads")
{
    SharedCache<int> cache;
    int buildCount = 0;
    const auto build = [&] {
        ++buildCount;
        return 7;
    };

    REQUIRE(cache.get(build) == 7);
    REQUIRE(cache.get(build) == 7);
    REQUIRE(buildCount == 1);
}

TEST_CASE("A copied shared cache shares the built value")
{
    SharedCache<std::vector<int>> original;
    const std::vector<int>& originalValue = original.get([] { return std::vector<int>{1, 2, 3}; });
    const SharedCache<std::vector<int>> copy = original;

    int buildCount = 0;
    const std::vector<int>& copiedValue = copy.get([&] {
        ++buildCount;
        return std::vector<int>{};
    });

    REQUIRE(&copiedValue == &originalValue);
    REQUIRE(buildCount == 0);
}

TEST_CASE("A cleared shared cache builds its value again on the next read")
{
    SharedCache<int> cache;
    cache.get([] { return 1; });

    cache.clear();

    REQUIRE(cache.get([] { return 2; }) == 2);
}

TEST_CASE("Clearing a copied shared cache leaves the original's value")
{
    SharedCache<int> original;
    original.get([] { return 1; });
    SharedCache<int> copy = original;

    copy.clear();
    copy.get([] { return 2; });

    int buildCount = 0;
    const int originalValue = original.get([&] {
        ++buildCount;
        return 3;
    });
    REQUIRE(originalValue == 1);
    REQUIRE(buildCount == 0);
}

TEST_CASE("A shared cache read from many threads builds its value once")
{
    SharedCache<int> cache;
    std::atomic<int> buildCount = 0;

    tbb::parallel_for(0, 10000, [&](int) {
        cache.get([&] {
            ++buildCount;
            return 7;
        });
    });

    REQUIRE(buildCount == 1);
}
