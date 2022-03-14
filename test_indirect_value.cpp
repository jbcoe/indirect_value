#include <string_view>

#include "indirect_value.h"

#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"

using isocpp_p1950::bad_indirect_value_access;
using isocpp_p1950::indirect_value;

/*! Helper function to capture constexpr results in catch test reporting.
    \note
        Credit to Jason Turner:
   https://twitter.com/lefticus/status/980530307580514304 \tparam B Compile time
   condition. \return The compile time condition result.
 */
template <bool B>
constexpr bool static_test() {
  static_assert(B);
  return B;
}

TEST_CASE("Ensure that indirect_value uses the minimum space requirements",
          "[indirect_value.sizeof]") {
  REQUIRE(static_test<sizeof(indirect_value<int>) ==
                      sizeof(std::unique_ptr<int>)>());
}

template <typename T>
class copy_counter {
 public:
  T* operator()(const T& rhs) const {
    ++call_count;
    return isocpp_p1950::default_copy<T>().operator()(rhs);
  }
  inline static size_t call_count = 0;
};

template <typename T>
class delete_counter {
 public:
  void operator()(T* rhs) const {
    ++call_count;
    return std::default_delete<T>().operator()(rhs);
  }
  inline static size_t call_count = 0;
};

TEST_CASE("Default construction for indirect_value", "[constructor.default]") {
  GIVEN("An indirect_value value")  // The ability to track internal copies and
                                    // deletes of the default constructor")
  {
    WHEN("Default-constructed") {
      indirect_value<int, copy_counter<int>, delete_counter<int>> a{};
      REQUIRE(a.operator->() == nullptr);

      THEN("Ensure no copies or deletes occur") {
        REQUIRE(copy_counter<int>::call_count == 0);
        REQUIRE(delete_counter<int>::call_count == 0);
      }
    }
    WHEN("The default-constructed value is destroyed") {
      THEN("Ensure no delete operation occurs") {
        // Expect a delete not to occur on destruction as the indirect_value was
        // default initialised
        REQUIRE(copy_counter<int>::call_count == 0);
        CHECK(delete_counter<int>::call_count == 0);
      }
    }
  }
  GIVEN("An indirect_value value")  //"The ability to track internal copies and
                                    // deletes of the default constructor")
  {
    WHEN(
        "Default constructed then copy assigned from a pointer-initialised")  //("Create a default-constructed indirect_value which is later copy-constructed")
    {
      indirect_value<int, copy_counter<int>, delete_counter<int>> a{};
      constexpr int b_value = 10;
      indirect_value<int, copy_counter<int>, delete_counter<int>> b{
          new int(b_value)};
      REQUIRE(a.operator->() == nullptr);
      REQUIRE(b.operator->() != nullptr);
      REQUIRE(*b == b_value);

      THEN("Ensure a copy occures but no delete occurs") {
        a = b;
        REQUIRE(copy_counter<int>::call_count == 1);
        REQUIRE(delete_counter<int>::call_count == 0);
      }
    }
    WHEN("The value is destroyed") {
      THEN("Ensure a delete operation occurs") {
        // Expect both indirect_value object to be deleted on destruction .
        REQUIRE(copy_counter<int>::call_count == 1);
        CHECK(delete_counter<int>::call_count == 2);
      }
    }
  }
}

TEST_CASE("Element wise initialisation construction for indirect_value",
          "[constructor.element_wise]") {
  GIVEN("The ability to track intenal copies and deletes") {
    size_t copy_count = 0, delete_count = 0;
    const auto copy_counter = [&copy_count](const auto& rhs) {
      ++copy_count;
      return isocpp_p1950::default_copy<
                 std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>()
          .
          operator()(rhs);
    };

    const auto delete_counter = [&delete_count](auto* rhs) {
      ++delete_count;
      std::default_delete<
          std::remove_cv_t<std::remove_pointer_t<decltype(rhs)>>>()
          .
          operator()(rhs);
    };

    WHEN("Constructing objects of indirect_value") {
      indirect_value<int, decltype(copy_counter), decltype(delete_counter)> a{
          new int(0), copy_counter, delete_counter};
      REQUIRE(a.operator->() != nullptr);

      THEN(
          "Ensure that no copies or deleted happen in the basic construction "
          "of a value") {
        REQUIRE(copy_count == 0);
        REQUIRE(delete_count == 0);
      }
    }

    // Ensure destruction of an indirect_value caused the value to be deleted
    REQUIRE(copy_count == 0);
    REQUIRE(delete_count == 1);
  }
}

TEST_CASE("Copy construction for indirect_value of a primitive type",
          "[constructor.copy.primitive]") {
  GIVEN("A value-initialised indirect_value value") {
    constexpr int a_value = 5;
    indirect_value<int> a{new int(a_value)};
    REQUIRE(*a == a_value);

    WHEN("Taking a copy of the value-initialised indirect_value value") {
      indirect_value<int> copy_of_a{a};
      THEN("The copy is a deep copy of the original value") {
        REQUIRE(*copy_of_a == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(copy_of_a.operator->() != nullptr);
        REQUIRE(a.operator->() != copy_of_a.operator->());
      }
    }
  }
}

TEST_CASE("Copy assignment for indirect_value of a primitive type",
          "[assignment.copy.primitive]") {
  GIVEN("A value-initialised indirect_value value") {
    constexpr int a_value = 5;
    indirect_value<int> a{new int(a_value)};
    REQUIRE(*a == a_value);

    WHEN("Assigning a copy into a default-initalised indirect_value value") {
      indirect_value<int> b{};
      REQUIRE(b.operator->() == nullptr);

      THEN("The assigned to object makes a deep copy of the orginal value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
    WHEN("Assigning a copy into a value-initalised indirect_value value") {
      constexpr int b_value = 10;
      indirect_value<int> b{std::in_place, b_value};
      REQUIRE(*b == b_value);

      THEN("The assigned to object makes a deep copy of the original value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
    WHEN("Assigning a copy into a pointer-initalised indirect_value value") {
      constexpr int b_value = 10;
      indirect_value<int> b{new int(b_value)};
      REQUIRE(*b == b_value);

      THEN("The assigned to object makes a deep copy of the original value") {
        b = a;
        REQUIRE(*b == a_value);
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(b.operator->() != nullptr);
        REQUIRE(b.operator->() != a.operator->());
      }
    }
  }
}

TEST_CASE("Move construction for indirect_value of a primitive type",
          "[constructor.move.primitive]") {
  GIVEN("A value-initalised indirect_value value") {
    constexpr int a_value = 5;
    indirect_value<int> a{new int(a_value)};

    WHEN("Constucting a new object via moving the orignal value") {
      int const* const location_of_a = a.operator->();
      indirect_value<int> b{std::move(a)};

      THEN(
          "The constructed object steals the contents of original value "
          "leaving it in a null state") {
        REQUIRE(*b == a_value);
        REQUIRE(b.operator->() == location_of_a);
        REQUIRE(a.operator->() == nullptr);
      }
    }
  }
}

TEST_CASE("Move assignment for indirect_value of a primitive type",
          "[assignment.move.primitive]") {
  GIVEN("A two value-initialised indirect_value values") {
    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect_value<int> a{new int(a_value)};
    indirect_value<int> b{new int(b_value)};

    WHEN(
        "The contents of the second indirect_value is move assigned to the "
        "first") {
      int const* const location_of_b = b.operator->();
      a = std::move(b);

      THEN(
          "The move-assigned-to value `a` steals the contents of the second "
          "value `b`, leaving that object, `b`, in a null state") {
        REQUIRE(*a == b_value);
        REQUIRE(a.operator->() == location_of_b);
        REQUIRE(b.operator->() == nullptr);
      }
    }
  }
}

TEST_CASE("Operator bool for indirect_value", "[operator.bool]") {
  GIVEN("A default-initalised indirect_value value") {
    indirect_value<int> a;

    WHEN(
        "We expect the operator bool to return false as the internal pointer "
        "is null") {
      REQUIRE(a.operator->() == nullptr);
      REQUIRE_FALSE(a);

      THEN(
          "Then when it is assigned a valid value for operator bool should "
          "return true") {
        constexpr int b_value = 10;
        a = indirect_value(new int(b_value));
        REQUIRE(a.operator->() != nullptr);
        REQUIRE(*a == b_value);
        REQUIRE(a);
      }
    }
  }
  GIVEN("A pointer-initialised indirect_value value") {
    constexpr int value_a = 7;
    indirect_value<int> a{new int(value_a)};

    WHEN(
        "We expect the operator bool to return true as the internal pointer "
        "owns an instance") {
      REQUIRE(a.operator->() != nullptr);
      REQUIRE(a);

      THEN(
          "Then when it is assigned a default state value for operator bool "
          "should return false") {
        a = indirect_value<int>{};
        REQUIRE(a.operator->() == nullptr);
        REQUIRE_FALSE(a);
      }
    }
  }
}

TEST_CASE("Swap overload for indirect_value", "[swap.primitive]") {
  GIVEN(
      "A two value-initialised indirect_value values utilising the empty "
      "base-class optimisation") {
    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect_value<int> a{new int(a_value)};
    indirect_value<int> b{new int(b_value)};

    WHEN("The contents are swap") {
      swap(a, b);

      THEN("The contents of the indirect_value should be moved") {
        REQUIRE(*a == b_value);
        REQUIRE(*b == a_value);
      }
    }
  }
  GIVEN(
      "A two value-initialised indirect_value values not using the empty "
      "base-class optimisation") {
    auto default_copy_lambda_a = [](int original) { return new int(original); };
    auto default_copy_lambda_b = [](int original) { return new int(original); };

    constexpr int a_value = 5;
    constexpr int b_value = 10;
    indirect_value<int, decltype(+default_copy_lambda_a)> a{
        new int(a_value), default_copy_lambda_a};
    indirect_value<int, decltype(+default_copy_lambda_b)> b{
        new int(b_value), default_copy_lambda_b};

    THEN(
        "Confirm sized base class is used and its size requirements meet our "
        "expectations") {
      REQUIRE(
          static_test<sizeof(decltype(a)) != sizeof(indirect_value<int>)>());
      REQUIRE(
          static_test<sizeof(decltype(b)) != sizeof(indirect_value<int>)>());
      REQUIRE(static_test<sizeof(decltype(a)) ==
                          (sizeof(indirect_value<int>) +
                           sizeof(decltype(+default_copy_lambda_a)))>());
      REQUIRE(static_test<sizeof(decltype(b)) ==
                          (sizeof(indirect_value<int>) +
                           sizeof(decltype(+default_copy_lambda_b)))>());
    }

    WHEN("The contents are swap") {
      swap(a, b);

      THEN("The contents of the indirect_value should be moved") {
        REQUIRE(*a == b_value);
        REQUIRE(*b == a_value);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Noexcept of observers", "[TODO]", indirect_value<int>&,
                   const indirect_value<int>&, indirect_value<int>&&,
                   const indirect_value<int>&&) {
  using T = TestType;
  static_assert(noexcept(std::declval<T>().operator->()));
  static_assert(noexcept(std::declval<T>().operator*()));
  static_assert(!noexcept(std::declval<T>().value()));
  static_assert(noexcept(std::declval<T>().operator bool()));
  static_assert(noexcept(std::declval<T>().has_value()));
  static_assert(noexcept(std::declval<T>().get_copier()));
  static_assert(noexcept(std::declval<T>().get_deleter()));
}

template <class T, class U>
inline constexpr bool same_const_qualifiers =
    std::is_const_v<std::remove_reference_t<T>> ==
    std::is_const_v<std::remove_reference_t<U>>;

template <class T, class U>
inline constexpr bool same_ref_qualifiers = false;

template <class T, class U>
inline constexpr bool same_ref_qualifiers<T&, U&> = true;

template <class T, class U>
inline constexpr bool same_ref_qualifiers<T&&, U&&> = true;

template <class T, class U>
inline constexpr bool same_const_and_ref_qualifiers =
    same_ref_qualifiers<T, U>&& same_const_qualifiers<T, U>;

TEMPLATE_TEST_CASE("Ref- and const-qualifier of observers", "[TODO]",
                   indirect_value<int>&, const indirect_value<int>&,
                   indirect_value<int>&&, const indirect_value<int>&&) {
  using T = TestType;

  static_assert(
      same_const_and_ref_qualifiers<T,
                                    decltype(std::declval<T>().operator*())>);
  static_assert(
      same_const_and_ref_qualifiers<T, decltype(std::declval<T>().value())>);
  static_assert(
      std::is_same_v<bool, decltype(std::declval<T>().operator bool())>);
  static_assert(std::is_same_v<bool, decltype(std::declval<T>().has_value())>);
  static_assert(
      same_const_qualifiers<T, decltype(std::declval<T>().get_copier())>);
  static_assert(
      same_const_qualifiers<T, decltype(std::declval<T>().get_deleter())>);
}

TEST_CASE("Test properties of bad_indirect_value_access", "[TODO]") {
  bad_indirect_value_access ex;
  // check that we can throw a bad_indirect_value_access and catch
  // it as const std::exception&.
  try {
    throw ex;
  } catch (const std::exception& e) {
    // Use std::string_view to get the correct behavior of operator==.
    std::string_view what = e.what();
    REQUIRE(what == ex.what());
    REQUIRE(what.size() > 0);
  }

  static_assert(std::is_base_of_v<std::exception, bad_indirect_value_access>);
  static_assert(
      std::is_nothrow_default_constructible_v<bad_indirect_value_access>);
  static_assert(
      std::is_nothrow_copy_constructible_v<bad_indirect_value_access>);
  static_assert(noexcept(ex.what()));
}

TEST_CASE("Calling value on empty indirect_value will throw", "[TODO]") {
  GIVEN("An empty indirect_value") {
    indirect_value<int> iv;
    THEN("Calling value will throw") {
      REQUIRE(!iv.has_value());
      REQUIRE_THROWS_AS(iv.value(), bad_indirect_value_access);
    }
  }

  GIVEN("An empty const indirect_value") {
    const indirect_value<int> iv;
    THEN("Calling value will throw") {
      REQUIRE(!iv.has_value());
      REQUIRE_THROWS_AS(iv.value(), bad_indirect_value_access);
    }
  }

  GIVEN("An empty indirect_value rvalue") {
    indirect_value<int> iv;
    THEN("Calling value will throw") {
      REQUIRE(!iv.has_value());
      REQUIRE_THROWS_AS(std::move(iv).value(), bad_indirect_value_access);
    }
  }

  GIVEN("An empty const indirect_value rvalue") {
    const indirect_value<int> iv;
    THEN("Calling value will throw") {
      REQUIRE(!iv.has_value());
      REQUIRE_THROWS_AS(std::move(iv).value(), bad_indirect_value_access);
    }
  }
}

TEST_CASE("get_copier returns modifiable lvalue reference", "[TODO]") {
  GIVEN("An lvalue of indirect_value with a modifiable copier") {
    struct Copier {
      std::string name;
      int* operator()(int x) const {
        REQUIRE(name == "Modified");
        return new int(x);
      }
    };

    indirect_value<int, Copier> iv(std::in_place, 10);
    THEN("Modifying the copier will be observable") {
      iv.get_copier().name = "Modified";
      REQUIRE(iv.get_copier().name == "Modified");
      iv = iv; //Force invocation of copier
    }
  }
}

TEST_CASE("get_deleter returns modifiable lvalue reference", "[TODO]") {
  GIVEN("An lvalue of indirect_value with a modifiable deleter") {
    struct Deleter {
      std::string name;
      void operator()(int* p) const {
        REQUIRE(name == "Modified");
        delete p;
      }
    };

    indirect_value<int, isocpp_p1950::default_copy<int>, Deleter> iv(
        std::in_place, 10);
    THEN("Modifying the deleter will be observable") {
      iv.get_deleter().name = "Modified";
      REQUIRE(iv.get_deleter().name == "Modified");
    }
  }
}

TEST_CASE("Relational operators between two indirect_values", "[TODO]") {
  GIVEN("Two empty indirect_value values") {
    const indirect_value<int> a;
    const indirect_value<int> b;

    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("One non-empty and one empty indirect_value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const indirect_value<int> empty;

    THEN("The values should be unequal") {
      REQUIRE(!(nonEmpty == empty));
      REQUIRE(nonEmpty != empty);
      REQUIRE(!(nonEmpty < empty));
      REQUIRE(nonEmpty > empty);
      REQUIRE(!(nonEmpty <= empty));
      REQUIRE(nonEmpty >= empty);
    }
  }

  GIVEN("Two non-empty indirect_value values with equal values") {
    const indirect_value<int> a(std::in_place, 0);
    const indirect_value<int> b(std::in_place, 0);
    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("Two non-empty indirect_value values with different values") {
    const indirect_value<int> a(std::in_place, 0);
    const indirect_value<int> b(std::in_place, 1);
    THEN("a should be less than b") {
      REQUIRE(!(a == b));
      REQUIRE(a != b);
      REQUIRE(a < b);
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(!(a >= b));
    }
  }
}

TEST_CASE("Relational operators between two indirect_values of different type",
          "[TODO]") {
  GIVEN("Two empty indirect_value values") {
    const indirect_value<int> a;
    const indirect_value<short> b;

    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("One non-empty and one empty indirect_value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const indirect_value<short> empty;

    THEN("The values should be unequal") {
      REQUIRE(!(nonEmpty == empty));
      REQUIRE(nonEmpty != empty);
      REQUIRE(!(nonEmpty < empty));
      REQUIRE(nonEmpty > empty);
      REQUIRE(!(nonEmpty <= empty));
      REQUIRE(nonEmpty >= empty);
    }
  }

  GIVEN("Two non-empty indirect_value values with equal values") {
    const indirect_value<int> a(std::in_place, 0);
    const indirect_value<short> b(std::in_place, short{0});
    THEN("The values should be equal") {
      REQUIRE(a == b);
      REQUIRE(!(a != b));
      REQUIRE(!(a < b));
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(a >= b);
    }
  }

  GIVEN("Two non-empty indirect_value values with different values") {
    const indirect_value<int> a(std::in_place, 0);
    const indirect_value<short> b(std::in_place, short{1});
    THEN("a should be less than b") {
      REQUIRE(!(a == b));
      REQUIRE(a != b);
      REQUIRE(a < b);
      REQUIRE(!(a > b));
      REQUIRE(a <= b);
      REQUIRE(!(a >= b));
    }
  }
}

TEST_CASE("Relational operators between an indirect_value and nullptr",
          "[TODO]") {
  GIVEN("An empty indirect_value") {
    const indirect_value<int> empty;

    THEN("The value should be equal to nullptr") {
      REQUIRE(empty == nullptr);
      REQUIRE(nullptr == empty);
      REQUIRE(!(empty != nullptr));
      REQUIRE(!(nullptr != empty));
      REQUIRE(!(empty < nullptr));
      REQUIRE(!(nullptr < empty));
      REQUIRE(!(empty > nullptr));
      REQUIRE(!(nullptr > empty));
      REQUIRE(empty <= nullptr);
      REQUIRE(nullptr <= empty);
      REQUIRE(empty >= nullptr);
      REQUIRE(nullptr >= empty);
    }
  }

  GIVEN("A non-empty indirect_value") {
    const indirect_value<int> nonEmpty(std::in_place);

    THEN("The value should be unequal to nullptr") {
      REQUIRE(!(nonEmpty == nullptr));
      REQUIRE(!(nullptr == nonEmpty));
      REQUIRE(nonEmpty != nullptr);
      REQUIRE(nullptr != nonEmpty);
      REQUIRE(!(nonEmpty < nullptr));
      REQUIRE(nullptr < nonEmpty);
      REQUIRE(nonEmpty > nullptr);
      REQUIRE(!(nullptr > nonEmpty));
      REQUIRE(!(nonEmpty <= nullptr));
      REQUIRE(nullptr <= nonEmpty);
      REQUIRE(nonEmpty >= nullptr);
      REQUIRE(!(nullptr >= nonEmpty));
    }
  }
}

TEST_CASE("Relational operators between indirect_value and value_type",
          "[TODO]") {
  GIVEN("An empty indirect_value and a value_type") {
    const indirect_value<int> empty;
    const int value{};

    THEN("The value should be greater") {
      REQUIRE(!(empty == value));
      REQUIRE(!(value == empty));
      REQUIRE(empty != value);
      REQUIRE(value != empty);
      REQUIRE(empty < value);
      REQUIRE(!(value < empty));
      REQUIRE(!(empty > value));
      REQUIRE(value > empty);
      REQUIRE(empty <= value);
      REQUIRE(!(value <= empty));
      REQUIRE(!(empty >= value));
      REQUIRE(value >= empty);
    }
  }

  GIVEN("A non-empty indirect_value and a value_type with equal value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const int value{};

    THEN("The value should be equal") {
      REQUIRE(nonEmpty == value);
      REQUIRE(value == nonEmpty);
      REQUIRE(!(nonEmpty != value));
      REQUIRE(!(value != nonEmpty));
      REQUIRE(!(nonEmpty < value));
      REQUIRE(!(value < nonEmpty));
      REQUIRE(!(nonEmpty > value));
      REQUIRE(!(value > nonEmpty));
      REQUIRE(nonEmpty <= value);
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(value >= nonEmpty);
    }
  }

  GIVEN("A non-empty indirect_value and a value_type with smaller value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const int value{-1};

    THEN("The value should be smaller") {
      REQUIRE(!(nonEmpty == value));
      REQUIRE(!(value == nonEmpty));
      REQUIRE(nonEmpty != value);
      REQUIRE(value != nonEmpty);
      REQUIRE(!(nonEmpty < value));
      REQUIRE(value < nonEmpty);
      REQUIRE(nonEmpty > value);
      REQUIRE(!(value > nonEmpty));
      REQUIRE(!(nonEmpty <= value));
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(!(value >= nonEmpty));
    }
  }
}

TEST_CASE(
    "Relational operators between indirect_value and value_type of different "
    "type",
    "[TODO]") {
  GIVEN("An empty indirect_value and a value_type") {
    const indirect_value<int> empty;
    const short value{};

    THEN("The value should be greater") {
      REQUIRE(!(empty == value));
      REQUIRE(!(value == empty));
      REQUIRE(empty != value);
      REQUIRE(value != empty);
      REQUIRE(empty < value);
      REQUIRE(!(value < empty));
      REQUIRE(!(empty > value));
      REQUIRE(value > empty);
      REQUIRE(empty <= value);
      REQUIRE(!(value <= empty));
      REQUIRE(!(empty >= value));
      REQUIRE(value >= empty);
    }
  }

  GIVEN("A non-empty indirect_value and a value_type with equal value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const short value{};

    THEN("The value should be equal") {
      REQUIRE(nonEmpty == value);
      REQUIRE(value == nonEmpty);
      REQUIRE(!(nonEmpty != value));
      REQUIRE(!(value != nonEmpty));
      REQUIRE(!(nonEmpty < value));
      REQUIRE(!(value < nonEmpty));
      REQUIRE(!(nonEmpty > value));
      REQUIRE(!(value > nonEmpty));
      REQUIRE(nonEmpty <= value);
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(value >= nonEmpty);
    }
  }

  GIVEN("A non-empty indirect_value and a value_type with smaller value") {
    const indirect_value<int> nonEmpty(std::in_place, 0);
    const short value{-1};

    THEN("The value should be smaller") {
      REQUIRE(!(nonEmpty == value));
      REQUIRE(!(value == nonEmpty));
      REQUIRE(nonEmpty != value);
      REQUIRE(value != nonEmpty);
      REQUIRE(!(nonEmpty < value));
      REQUIRE(value < nonEmpty);
      REQUIRE(nonEmpty > value);
      REQUIRE(!(value > nonEmpty));
      REQUIRE(!(nonEmpty <= value));
      REQUIRE(value <= nonEmpty);
      REQUIRE(nonEmpty >= value);
      REQUIRE(!(value >= nonEmpty));
    }
  }
}

#ifdef __cpp_concepts

template <class T, class U, class Comp>
concept Compare = requires(const T& a, const U& b) {
  {Comp{}(a, b)};
  {Comp{}(b, a)};
};

TEST_CASE(
    "Relational operators between indirect_value and value_type of "
    "non-equality-comparable type",
    "[TODO]") {
  struct NonComparable {};
  using IV = indirect_value<NonComparable>;
  static_assert(!Compare<IV, NonComparable, std::equal_to<>>);
  static_assert(!Compare<IV, NonComparable, std::not_equal_to<>>);
  static_assert(!Compare<IV, NonComparable, std::less<>>);
  static_assert(!Compare<IV, NonComparable, std::greater<>>);
  static_assert(!Compare<IV, NonComparable, std::less_equal<>>);
  static_assert(!Compare<IV, NonComparable, std::greater_equal<>>);
}

#endif
