#include <catch2/catch.hpp>

unsigned int fac(unsigned int number) {
    return number <= 1 ? number : fac(number - 1) * number;
}

TEST_CASE("Factorials are computed", "[factorial]") {
    REQUIRE( fac(1) == 1 );
    REQUIRE( fac(2) == 2 );
    REQUIRE( fac(3) == 6 );
    REQUIRE( fac(10) == 3628800 );
}
