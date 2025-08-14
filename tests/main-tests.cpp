#include <catch2/catch_test_macros.hpp>
#include <tbb/parallel_for.h>
#include <iostream>
#include <boost/dll/import.hpp>
#include "Engine/Network/NetworkManager.h"
#include "Engine/Operator/OperatorTable.h"

TEST_CASE("foo")
{
    REQUIRE(1==1);
}

TEST_CASE("tbb")
{
    constexpr int N = 100;

    tbb::parallel_for(0, N, [](int i) {
        std::cout << "Iteration " << i
                  << " is running on thread "
                  << std::this_thread::get_id() << std::endl;
    });
}
