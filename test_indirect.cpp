#include "indirect.h"

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

using jbcoe::indirect;

template <typename T>
class copy_counter {
public:
    T* operator()(const T& rhs)
    {
        ++call_count;
        return jbcoe::default_copy<T>().operator()(rhs);
    }
    inline static size_t call_count = 0;
};
        
template <typename T>
class delete_counter {
public:
    void operator()(T* rhs)
    {
        ++call_count;
        return std::default_delete<T>().operator()(rhs);
    }
    inline static size_t call_count = 0;
};
 
TEST_CASE("Default construction for indirect", "[constructor.default]")
{   
    GIVEN("The ability to track internal copies and deletes of the default constructor")
    {
   
        WHEN("Initailising a default constructor")
        {
            indirect<int, copy_counter<int>, delete_counter<int>> a{};
            REQUIRE(a.operator->() == nullptr);

            THEN("Ensure no copies or deletes occur")
            {
                REQUIRE(copy_counter<int>::call_count == 0);
                REQUIRE(delete_counter<int>::call_count == 0);
            }
            THEN("Expect a delete no to occur on destruction as the indirect was default initialised")
            {
                a.~indirect();
                REQUIRE(copy_counter<int>::call_count == 0);
                CHECK(delete_counter<int>::call_count == 0);
            }
        }
    }
}

TEST_CASE("Element wise initialisation construction for indirect", "[constructor.element_wise]")
{  
    GIVEN("The ability to track intenal copies and deletes")
    {
        size_t copy_count = 0, delete_count = 0;
        const auto copy_counter= [&copy_count](const auto& rhs)
        { 
            ++copy_count; 
            return jbcoe::default_copy<std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>().operator()(rhs);
        };

        const auto delete_counter= [&delete_count](auto* rhs)
        { 
            ++delete_count; 
            std::default_delete<std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>().operator()(rhs);
        };

        WHEN("Constructing objects of indirect")
        {
            indirect<int, decltype(copy_counter), decltype(delete_counter)> a{new int(0), copy_counter, delete_counter};
            REQUIRE(a.operator->() != nullptr); 
            
            THEN("Ensure that no copies or deleted happen in the basic construction of a value")
            {
                REQUIRE(copy_count==0);
                REQUIRE(delete_count==0);
            }
            THEN("Ensure destruction of an indirect caused the value to be deleted")
            {
                a.~indirect();
                REQUIRE(copy_count==0);
                REQUIRE(delete_count==1);
            }
        }
    }
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
             indirect<int> b{std::in_place, b_value};
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
        WHEN("Assigning a copy into a pointer-initalised indirect value")
        {
             constexpr int b_value = 10;
             indirect<int> b{ new int(b_value) };
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
        constexpr int a_value = 5;
        constexpr int b_value = 10;
        indirect<int> a{ new int(a_value) };
        indirect<int> b{ new int(b_value) };

        WHEN("The contents of the second indirect is move assigned to the first")
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

TEST_CASE("Operator bool for indirect", "[operator.bool]")
{ 
    GIVEN("A default-initalised indirect value")
    {
        indirect<int> a;

        WHEN("We expect the operator bool to return false as the internal pointer is null")
        {
            REQUIRE(a.operator->() == nullptr);
            REQUIRE_FALSE(a);

            THEN("Then when it is assigned a valid value for operator bool should return true")
            {
                constexpr int b_value = 10; 
                a = indirect(new int(b_value));
                REQUIRE(a.operator->() != nullptr);
                REQUIRE(*a == b_value);
                REQUIRE(a);
            }
        }
    }
    GIVEN("A pointer-initalised indirect value")
    {
        constexpr int value_a = 7;
        indirect<int> a{ new int (value_a) };

        WHEN("We expect the operator bool to return true as the internal pointer owns an instance")
        {
            REQUIRE(a.operator->() != nullptr);
            REQUIRE(a);

            THEN("Then when it is assigned a default state value for operator bool should return false")
            { 
                a = indirect<int>{};
                REQUIRE(a.operator->() == nullptr);
                REQUIRE_FALSE(a);
            }
        }
    }
}
