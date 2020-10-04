#include "catch2/catch.hpp"
#include "example_pimpl.h"

TEST_CASE(
    "Basic life time operations of a pimpl now work for free via "
    "indirect_value via the rule of zero",
    "[example_pimpl.life_cycle]") {
  GIVEN("An instance of the pimpl type") {
    const std::string nameA = "First Pimpl";
    example_pimpl a;
    a.set_name(nameA.c_str());

    WHEN("Copying constructing an instance.") {
      example_pimpl b(a);

      THEN("Ensure the copied class mirrors the original") {
        REQUIRE(std::string(a.get_name()) == std::string(b.get_name()));
        REQUIRE(b.is_valid() == true);
      }
    }
    WHEN("Moving constructing an instance.") {
      const auto string_location = a.get_name();
      example_pimpl b(std::move(a));

      THEN("Ensure the moved class has the contents of the original") {
        REQUIRE(std::string(b.get_name()) == std::string(string_location));
        REQUIRE(a.is_valid() == false);
      }
    }
    WHEN("Copying assigning across to a default constructed instance.") {
      const std::string nameB = "Second Pimpl";
      example_pimpl b(nameB.c_str());
      b = a;

      THEN("Ensure the copied class mirrors the original") {
        REQUIRE(std::string(a.get_name()) == std::string(b.get_name()));
        REQUIRE(b.is_valid() == true);
      }
    }
    WHEN("Moving assigning across to a default constructed instance.") {
      example_pimpl b;
      const auto string_location = a.get_name();
      b = std::move(a);

      THEN("Ensure the moved class has the contents of the original") {
        REQUIRE(std::string(b.get_name()) == std::string(string_location));
        REQUIRE(a.is_valid() == false);
      }
    }
  }
}