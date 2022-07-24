/* Copyright (c) 2019 The Indirect Value Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#include "pimpl.h"

#include "catch2/catch.hpp"

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
      }
    }
    WHEN("Moving constructing an instance.") {
      const auto string_location = a.get_name();
      example_pimpl b(std::move(a));

      THEN("Ensure the moved class has the contents of the original") {
        REQUIRE(std::string(b.get_name()) == std::string(string_location));
      }
    }
    WHEN("Copying assigning across to a default constructed instance.") {
      const std::string nameB = "Second Pimpl";
      example_pimpl b(nameB.c_str());
      b = a;

      THEN("Ensure the copied class mirrors the original") {
        REQUIRE(std::string(a.get_name()) == std::string(b.get_name()));
      }
    }
    WHEN("Moving assigning across to a default constructed instance.") {
      example_pimpl b;
      const auto string_location = a.get_name();
      b = std::move(a);

      THEN("Ensure the moved class has the contents of the original") {
        REQUIRE(std::string(b.get_name()) == std::string(string_location));
      }
    }
  }
}