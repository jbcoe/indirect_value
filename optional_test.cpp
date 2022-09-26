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

#include <catch2/catch.hpp>
#include <vector>

#include "indirect_value.h"

using isocpp_p1950::indirect_value;

TEST_CASE(
    "Ensure that optional<indirect_value> uses the minimum space requirements",
    "[optional<indirect_value>.sizeof]") {
  STATIC_REQUIRE(sizeof(std::optional<indirect_value<int>>) ==
                 sizeof(indirect_value<int>));
}

class NonConstructable {
  NonConstructable() = default;
};

TEST_CASE(
    "Verify optional indirect value support construction to the null-state by "
    "default initalisation",
    "[optional<indirect_value>.construction.default]") {
  GIVEN("A non constructable type") {
    WHEN("Use nullopt construct of an optional of indirect_value") {
      std::optional<indirect_value<NonConstructable>> value;

      THEN("Expect it to be initialised to its null-state") { REQUIRE(!value); }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction to the null-state by "
    "initalisation from nullopt sentinel value",
    "[optional<indirect_value>.construction.nullopt]") {
  GIVEN("A non constructable type") {
    WHEN("Use nullopt construct of an optional of indirect_value") {
      std::optional<indirect_value<NonConstructable>> value(std::nullopt);

      THEN("Expect it to be initialised to its null-state") { REQUIRE(!value); }
    }
    WHEN("Use implicit nullopt construct of an optional of indirect_value") {
      std::optional<indirect_value<NonConstructable>> value = std::nullopt;

      THEN("Expect it to be initialised to its null-state") { REQUIRE(!value); }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction by copy construction",
    "[optional<indirect_value>.construction.copy]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> copy(initial);
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
    WHEN("Implicit copy constructing from non null-state") {
      std::optional<indirect_value<int>> copy = initial;
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<NonConstructable>> initial;
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<NonConstructable>> copy(initial);
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!copy);
      }
    }
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<NonConstructable>> copy = initial;
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!copy);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction by move construction",
    "[optional<indirect_value>.construction.move]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    WHEN("Move constructing from non null-state") {
      std::optional<indirect_value<int>> copy(std::move(initial));
      THEN("Resulting move should also have non null-state") {
        REQUIRE(!initial);
        REQUIRE(copy);
      }
    }
    WHEN("Implicit move constructing from non null-state") {
      std::optional<indirect_value<int>> copy = std::move(initial);
      THEN("Resulting move should also have non null-state") {
        REQUIRE(!initial);
        REQUIRE(copy);
      }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<NonConstructable>> initial;
    WHEN("Move constructing from null-state") {
      std::optional<indirect_value<NonConstructable>> copy(std::move(initial));
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!copy);
      }
    }
    WHEN("Move constructing from null-state") {
      std::optional<indirect_value<NonConstructable>> copy = std::move(initial);
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!copy);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction by "
    "conversion copy construction from optional",
    "[optional<indirect_value>.construction.copy.convert_from_optional]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<int> initial(10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> copy(initial);
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
    WHEN("Implicit copy constructing from non null-state") {
      std::optional<int> copy = initial;
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction by "
    "conversion move construction from optional",
    "[optional<indirect_value>.construction.move.convert_from_optional]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<int> initial(std::in_place, 10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> copy(std::move(initial));
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
    WHEN("Implicit copy constructing from non null-state") {
      std::optional<int> copy = std::move(initial);
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
  }
}

TEST_CASE("Verify optional indirect value support inplace construction",
          "[optional<indirect_value>.construction.inplace]") {
  GIVEN("An in_place constucted optional indirect value") {
    std::optional<indirect_value<int>> value(std::in_place, 10);
    WHEN("Accessing the value") {
      THEN(
          "The underlying value should be initialised from the input "
          "parameters") {
        REQUIRE(*value == 10);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support inplace construction with an "
    "initialiser list",
    "[optional<indirect_value>.construction.inplace_with_initialiser_list]") {
  GIVEN("An in_place constucted optional indirect value") {
    std::optional<indirect_value<std::vector<int>>> value(std::in_place,
                                                          {1, 2, 3, 4, 5});
    WHEN("Accessing the value") {
      THEN(
          "The underlying value should be initialised from the input "
          "parameters") {
        REQUIRE((*value)[0] == 1);
        REQUIRE((*value)[1] == 2);
        REQUIRE((*value)[2] == 3);
        REQUIRE((*value)[3] == 4);
        REQUIRE((*value)[4] == 5);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support construction by "
    "move construction from other types",
    "[optional<indirect_value>.construction.move.convert_from_other]") {
  GIVEN("A non empty optional indirect value") {
    short int initial(10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> moved(std::move(initial));
      THEN("Resulting moved to object should also hold the inital value") {
        REQUIRE(moved == 10);
      }
    }
    WHEN("Implicit copy constructing from non null-state") {
      std::optional<int> moved = std::move(initial);
      THEN("Resulting moved to object should also hold the inital value") {
        REQUIRE(moved == 10);
      }
    }
  }
}

TEST_CASE("Verify optional indirect value support assignment to a nullopt",
          "[optional<indirect_value>.assignment.nullopt]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(10);
    WHEN("Assigning from non null-state") {
      initial = std::nullopt;
      THEN("Resulting value should have non null-state") { REQUIRE(!initial); }
    }
  }
}

TEST_CASE("Verify optional indirect value support copy assignment",
          "[optional<indirect_value>.assignment.copy]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    std::optional<indirect_value<int>> copy;
    WHEN("Copy assigning from valid-state") {
      copy = initial;
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
        REQUIRE(*copy == 10);
      }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<NonConstructable>> initial;
    std::optional<indirect_value<NonConstructable>> copy;
    WHEN("Copy assigning from null-state") {
      copy = initial;
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!copy);
      }
    }
  }
}

TEST_CASE("Verify optional indirect value support move assignment",
          "[optional<indirect_value>.assignment.move]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    std::optional<indirect_value<int>> moved;
    WHEN("Move constructing from non null-state") {
      moved = std::move(initial);
      THEN("Resulting move should also have non null-state") {
        REQUIRE(!initial);
        REQUIRE(moved);
      }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<NonConstructable>> initial;
    std::optional<indirect_value<NonConstructable>> moved;
    WHEN("Move constructing from null-state") {
      moved = std::move(initial);
      THEN("Resulting value should also have null-state") {
        REQUIRE(!initial);
        REQUIRE(!moved);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support assignment by "
    "move assignment from other types",
    "[optional<indirect_value>.assignment.move.convert_from_other]") {
  GIVEN("A non empty optional indirect value") {
    short int initial(10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> moved;
      moved = std::move(initial);
      THEN("Resulting moved to object should also hold the inital value") {
        REQUIRE(moved == 10);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support assignment by "
    "conversion copy assignment from optional",
    "[optional<indirect_value>.assignment.copy.convert_from_optional]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<int> initial(std::in_place, 10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> copy;
      copy = initial;
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
  }
}

TEST_CASE(
    "Verify optional indirect value support assignment by "
    "conversion move assignment from optional",
    "[optional<indirect_value>.assignment.move.convert_from_optional]") {
  GIVEN("A non empty optional indirect value") {
    std::optional<int> initial(std::in_place, 10);
    WHEN("Copy constructing from null-state") {
      std::optional<indirect_value<int>> copy;
      copy = std::move(initial);
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(initial);
        REQUIRE(copy);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Verify optional indirect value access value method",
                   "[optional<indirect_value>.value]",
                   (std::optional<indirect_value<int>>&),
                   (std::optional<indirect_value<int>> const&),
                   (std::optional<indirect_value<int>> &&),
                   (std::optional<indirect_value<int>> const&&)) {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    WHEN("Accessing the value") {
      auto const& value = static_cast<TestType>(initial).value();
      THEN("Resulting copy should also have non null-state") {
        REQUIRE(*value == 10);
      }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<int>> initial;
    WHEN("Accessing the value") {
      THEN("A bad access exception should be raised") {
        REQUIRE_THROWS_AS(static_cast<TestType>(initial).value(),
                          std::bad_optional_access);
      }
    }
  }
}

TEMPLATE_TEST_CASE("Verify optional indirect value access value_or method",
                   "[optional<indirect_value>.value_or]",
                   (std::optional<indirect_value<int>> const&),
                   (std::optional<indirect_value<int>> &&)) {
  GIVEN("A non empty optional indirect value") {
    std::optional<indirect_value<int>> initial(std::in_place, 10);
    WHEN("Accessing the value or a default") {
      auto const value = static_cast<TestType>(initial).value_or(500);
      THEN("expect the value to be returned") { REQUIRE(value == 10); }
    }
  }
  GIVEN("An empty optional indirect value") {
    std::optional<indirect_value<int>> initial;
    WHEN("Accessing the value or a default") {
      auto const value = static_cast<TestType>(initial).value_or(500);
      THEN("expect the value to be returned") { REQUIRE(value == 500); }
    }
  }
}
