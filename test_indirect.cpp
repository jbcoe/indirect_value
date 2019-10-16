#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "indirect.h"

using jbcoe::indirect;

TEST_CASE("Nothing to see here", "[dummy]") { REQUIRE(2 + 2 != 5); }

TEST_CASE("Defeault construction for indirect", "[constructor.default]")
{    
    indirect<int> a{};
    REQUIRE(a.operator->() == nullptr); 
}

TEST_CASE("Move construction for indirect of a primitive type", "[constructor.move.primitive]")
{
    const int a_value = 5;
    indirect<int> a{new int(a_value) }, b{ std::move(a) };
    REQUIRE(*b == a_value); 
    REQUIRE(a.operator->() == nullptr); 
}

TEST_CASE("Move assignment for indirect of a primative type", "[assignment.move.primitive]")
{
    const int a_value = 5, b_value = 10;
    indirect<int> a{ new int(a_value) }, b{ new int(b_value) };
    a = std::move(b);
    REQUIRE(*a == b_value); 
    REQUIRE(b.operator->() == nullptr); 
}
