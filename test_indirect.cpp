#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "indirect.h"

TEST_CASE("Nothing to see here", "[dummy]") {
  REQUIRE(2 + 2 != 5);
}
