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

TEST_CASE("Copy construction for indirect of a primitive type", "[constructor.copy.primitive]")
{
    GIVEN("A value-initialised indirect value")
    {
        constexpr int a_value = 5;
        indirect<int> a{ new int(a_value) };
        REQUIRE(*a == a_value);

        WHEN("Taking a copy of the value-initialised indirect value")
        {
            indirect<int> copy_of_a{ a };
            THEN("The copy is a deep copy of the orginal value")
            {
                REQUIRE(*copy_of_a == a_value); 
                REQUIRE(a.operator->() != nullptr); 
                REQUIRE(copy_of_a.operator->() != nullptr); 
                REQUIRE(a.operator->() != copy_of_a.operator->()); 
            }
        }
    }
}

TEST_CASE("Copy assignment for indirect of a primitive type", "[assignment.copy.primitive]")
{
    GIVEN("A value-initialised indirect value")
    {
        constexpr int a_value = 5;
        indirect<int> a{ new int(a_value) };
        REQUIRE(*a == a_value);
 
        WHEN("Assigning a copy into a default-initalised indirect value")
        {
             indirect<int> b{};
             REQUIRE(b.operator->() == nullptr);

             THEN("The assigned to object makes a deep copy of the orginal value")
             {
                 b = a;
                 REQUIRE(*b == a_value);
                 REQUIRE(a.operator->() != nullptr);
                 REQUIRE(b.operator->() != nullptr);
                 REQUIRE(b.operator->() != a.operator->());
             }
        }
        WHEN("Assigning a copy into a value-initalised indirect value")
        {
             constexpr int b_value = 10;
             indirect<int> b{ new int(b_value) };
             REQUIRE(*b == b_value);
             int const * const location_of_b = b.operator->();

             THEN("The assigned to object make a deep copy of the original value without futher unnecessary allocation")
             {
                 b = a;
                 REQUIRE(location_of_b == b.operator->());
                 REQUIRE(*b == a_value);
                 REQUIRE(a.operator->() != nullptr);
                 REQUIRE(b.operator->() != nullptr);
                 REQUIRE(b.operator->() != a.operator->());
             }
        }
    }
}

TEST_CASE("Move construction for indirect of a primitive type", "[constructor.move.primitive]")
{
    GIVEN("A value-initalised indirect value")
    {    
        constexpr int a_value = 5;
        indirect<int> a{new int(a_value) };
        
        WHEN("Constucting a new object via moving the orignal value")
        {
            int const * const location_of_a = a.operator->();
            indirect<int> b{ std::move(a) };

            THEN("The constructed object steals the contents of original value leaving it in a null state")
            {
                REQUIRE(*b == a_value); 
                REQUIRE(b.operator->() == location_of_a);
                REQUIRE(a.operator->() == nullptr);
            }
        }
    }
}

TEST_CASE("Move assignment for indirect of a primitive type", "[assignment.move.primitive]")
{
    GIVEN("A two value-initialised indirect values")
    {
        constexpr int a_value = 5, b_value = 10;
        indirect<int> a{ new int(a_value) }, b{ new int(b_value) };

        WHEN("The contents of the second it move assigned to the first")
        {
            int const * const location_of_b = b.operator->();
            a = std::move(b);

            THEN("The move assigned to value steals the contents of the second value, leaving that object in a null state")
            {
                REQUIRE(*a == b_value); 
                REQUIRE(a.operator->() == location_of_b);
                REQUIRE(b.operator->() == nullptr); 
            }
        }
    }
}
