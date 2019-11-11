#include "indirect_value.h"

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

using isocpp_p1950::indirect_value;
using isocpp_p1950::default_assign;

/*! Helper function to capture constexpr results in catch test reporting.
    \note
        Credit to Jason Turner: https://twitter.com/lefticus/status/980530307580514304
    \tparam B
        Compile time condition.
    \return
        The compile time condition result.
 */
template<bool B>
bool static_test()
{
    static_assert(B);
    return B;
}

// FIXME(Restore this test once we can get it to pass on MSVC)
// TEST_CASE("Ensure that indirect_value uses the minum space requirements", "[indirect_value.sizeof]")
// {
//     REQUIRE(static_test<sizeof(indirect_value<int>) == sizeof(std::unique_ptr<int>)>());
// }

template <typename T>
class copy_counter {
public:
    T* operator()(const T& rhs) const
    {
        ++call_count;
        return isocpp_p1950::default_copy<T>().operator()(rhs);
    }
    inline static size_t call_count = 0;
};
        
template <typename T>
class delete_counter {
public:
    void operator()(T* rhs) const
    {
        ++call_count;
        return std::default_delete<T>().operator()(rhs);
    }
    inline static size_t call_count = 0;
};
 
TEST_CASE("Default construction for indirect_value", "[constructor.default]")
{   
    GIVEN("An indirect_value")//The ability to track internal copies and deletes of the default constructor")
    {
        WHEN("Default-constructed")
        {
            indirect_value<int, copy_counter<int>, delete_counter<int>> a{};
            REQUIRE(a.operator->() == nullptr);

            THEN("Ensure no copies or deletes occur")
            {
                REQUIRE(copy_counter<int>::call_count == 0);
                REQUIRE(delete_counter<int>::call_count == 0);
            }
        }
        WHEN("The default-constructed value is destroyed")
        {
            THEN("Ensure no delete operation occurs")
            {
                // Expect a delete not to occur on destruction as the indirect_value was default initialised
                REQUIRE(copy_counter<int>::call_count == 0);
                CHECK(delete_counter<int>::call_count == 0);
            }
        }
    }
    GIVEN("An indirect_value") //"The ability to track internal copies and deletes of the default constructor")
    {
        WHEN("Default constructed then copy assigned from a pointer-initialised")//("Create a default-constructed indirect_value which is later copy-constructed")
        {
            indirect_value<int, copy_counter<int>, default_assign<int>, delete_counter<int>> a{};
            constexpr int b_value = 10;
            indirect_value<int, copy_counter<int>, default_assign<int>, delete_counter<int>> b{new int (b_value)};
            REQUIRE(a.operator->() == nullptr);
            REQUIRE(b.operator->() != nullptr);
            REQUIRE(*b == b_value);

            THEN("Ensure a copy occures but no delete occurs")
            {
                a=b;
                REQUIRE(copy_counter<int>::call_count == 1);
                REQUIRE(delete_counter<int>::call_count == 0);
            }
        }
        WHEN("The value is destroyed")
        {
            THEN("Ensure a delete operation occurs")
            {
                // Expect both indirect_value object to be deleted on destruction .
                REQUIRE(copy_counter<int>::call_count == 1);
                CHECK(delete_counter<int>::call_count == 2);
            }
        }
    }
}

TEST_CASE("Element wise initialisation construction for indirect_value", "[constructor.element_wise]")
{  
    GIVEN("The ability to track intenal copies and deletes")
    {
        size_t copy_count = 0, delete_count = 0;
        const auto copy_counter= [&copy_count](const auto& rhs)
        { 
            ++copy_count; 
            return isocpp_p1950::default_copy<std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>().operator()(rhs);
        };

        const auto delete_counter= [&delete_count](auto* rhs)
        { 
            ++delete_count; 
            std::default_delete<std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>().operator()(rhs);
        };

        WHEN("Constructing objects of indirect_value")
        {
            indirect_value<int, decltype(copy_counter), default_assign<int>, decltype(delete_counter)> a{new int(0), copy_counter, default_assign<int>{}, delete_counter};
            REQUIRE(a.operator->() != nullptr); 
            
            THEN("Ensure that no copies or deleted happen in the basic construction of a value")
            {
                REQUIRE(copy_count==0);
                REQUIRE(delete_count==0);
            }
        }
        
        // Ensure destruction of an indirect_value caused the value to be deleted
        REQUIRE(copy_count==0);
        REQUIRE(delete_count==1);
    }
}

TEST_CASE("Copy construction for indirect_value of a primitive type", "[constructor.copy.primitive]")
{
    GIVEN("A value-initialised indirect_value")
    {
        constexpr int a_value = 5;
        indirect_value<int> a{ new int(a_value) };
        REQUIRE(*a == a_value);

        WHEN("Taking a copy of the value-initialised indirect_value")
        {
            indirect_value<int> copy_of_a{ a };
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

TEST_CASE("Copy assignment for indirect_value of a primitive type", "[assignment.copy.primitive]")
{
    GIVEN("A value-initialised indirect_value")
    {
        constexpr int a_value = 5;
        indirect_value<int> a{ new int(a_value) };
        REQUIRE(*a == a_value);
 
        WHEN("Assigning a copy into a default-initalised indirect_value")
        {
             indirect_value<int> b{};
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
        WHEN("Assigning a copy into a value-initalised indirect_value")
        {
             constexpr int b_value = 10;
             indirect_value<int> b{std::in_place, b_value};
             REQUIRE(*b == b_value);

             THEN("The assigned to object makes a deep copy of the original value")
             {
                 b = a;
                 REQUIRE(*b == a_value);
                 REQUIRE(a.operator->() != nullptr);
                 REQUIRE(b.operator->() != nullptr);
                 REQUIRE(b.operator->() != a.operator->());
             }
        }
        WHEN("Assigning a copy into a pointer-initalised indirect_value")
        {
             constexpr int b_value = 10;
             indirect_value<int> b{ new int(b_value) };
             REQUIRE(*b == b_value);

             THEN("The assigned to object makes a deep copy of the original value")
             {
                 b = a;
                 REQUIRE(*b == a_value);
                 REQUIRE(a.operator->() != nullptr);
                 REQUIRE(b.operator->() != nullptr);
                 REQUIRE(b.operator->() != a.operator->());
             }
        }
        WHEN("Assigning to an empty indirect_value")
        {
             indirect_value<int> b;
             REQUIRE(!b);

             THEN("The assigned to object is empty")
             {
                 a = b;
                 REQUIRE(!a);
             }
        }
    }
}

TEST_CASE("Move construction for indirect_value of a primitive type", "[constructor.move.primitive]")
{
    GIVEN("A value-initalised indirect_value")
    {    
        constexpr int a_value = 5;
        indirect_value<int> a{new int(a_value) };
        
        WHEN("Constucting a new object via moving the orignal value")
        {
            int const * const location_of_a = a.operator->();
            indirect_value<int> b{ std::move(a) };

            THEN("The constructed object steals the contents of original value leaving it in a null state")
            {
                REQUIRE(*b == a_value); 
                REQUIRE(b.operator->() == location_of_a);
                REQUIRE(a.operator->() == nullptr);
            }
        }
    }
}

TEST_CASE("Move assignment for indirect_value of a primitive type", "[assignment.move.primitive]")
{
    GIVEN("A two value-initialised indirect_values")
    {
        constexpr int a_value = 5;
        constexpr int b_value = 10;
        indirect_value<int> a{ new int(a_value) };
        indirect_value<int> b{ new int(b_value) };

        WHEN("The contents of the second indirect_value is move assigned to the first")
        {
            int const * const location_of_b = b.operator->();
            a = std::move(b);

            THEN("The move-assigned-to value `a` steals the contents of the second value `b`, leaving that object, `b`, in a null state")
            {
                REQUIRE(*a == b_value); 
                REQUIRE(a.operator->() == location_of_b);
                REQUIRE(b.operator->() == nullptr); 
            }
        }
    }
}

TEST_CASE("Operator bool for indirect_value", "[operator.bool]")
{ 
    GIVEN("A default-initalised indirect_value")
    {
        indirect_value<int> a;

        WHEN("We expect the operator bool to return false as the internal pointer is null")
        {
            REQUIRE(a.operator->() == nullptr);
            REQUIRE_FALSE(a);

            THEN("Then when it is assigned a valid value for operator bool should return true")
            {
                constexpr int b_value = 10; 
                a = indirect_value(new int(b_value));
                REQUIRE(a.operator->() != nullptr);
                REQUIRE(*a == b_value);
                REQUIRE(a);
            }
        }
    }
    GIVEN("A pointer-initalised indirect_value")
    {
        constexpr int value_a = 7;
        indirect_value<int> a{ new int (value_a) };

        WHEN("We expect the operator bool to return true as the internal pointer owns an instance")
        {
            REQUIRE(a.operator->() != nullptr);
            REQUIRE(a);

            THEN("Then when it is assigned a default state value for operator bool should return false")
            { 
                a = indirect_value<int>{};
                REQUIRE(a.operator->() == nullptr);
                REQUIRE_FALSE(a);
            }
        }
    }
}
