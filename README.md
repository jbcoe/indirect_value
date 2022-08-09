# An indirect value-type for C++ (perfect-pimpl)

[![ConanCenter Package][badge.conan]][conan]
[![codecov][badge.codecov]][codecov]
[![language][badge.language]][language]
[![license][badge.license]][license]
[![issues][badge.issues]][issues]

[badge.conan]: https://repology.org/badge/version-for-repo/conancenter/indirect_value.svg
[badge.codecov]: https://img.shields.io/codecov/c/github/jbcoe/indirect_value/master.svg?logo=codecov
[badge.language]: https://img.shields.io/badge/language-C%2B%2B17-yellow.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.issues]: https://img.shields.io/github/issues/jbcoe/indirect_value.svg

[conan]: https://conan.io/center/indirect_value
[codecov]: https://codecov.io/gh/jbcoe/indirect_value
[language]: https://en.wikipedia.org/wiki/C%2B%2B17
[license]: https://en.wikipedia.org/wiki/MIT_License
[issues]: http://github.com/jbcoe/indirect_value/issues

The class template `indirect_value` is proposed for addition to the C++ Standard Library.

The class template, `indirect_value`, confers value-like semantics on a free-store
allocated object.  A `indirect_value<T>` may hold an object of class T, copying
the indirect_value<T> will copy the object of T, and const-ness will
propagate from the owning classes to the indirect_value type.

using `indirect_value` a composite class can be written as:

~~~ {.cpp}
#include "indirect_value.h"

class Composite {
  indirect_value<A> a_;
  indirect_value<B> b_;
};
~~~

When `A` and `B` can be incomplete types. 

If `A` and `B` are copyable then the compiler-generated copy constructor of `Composite` will copy
each of the components using their copy constructors. (Note: If `A` and `B` are base classes and `a_` and `b_` may store
derived-type objects then prefer (`polymorphic_value`)[https://github.com/jbcoe/polymorphic_value]).

`indirect_value` propagates `const` unlike `std::unique_ptr` so may be better choice for member data of a composite class.

```
#include <iostream>
#include <utility>

#include "indirect_value.h"
using isocpp_p1950::indirect_value;

struct Component {
  const char* foo() { return "non-const foo"; }
  const char* foo() const { return "const foo"; }
};

class UniquePtrComposite {
 public:
  UniquePtrComposite() : c_(new Component()) {}
  const char* foo() { return c_->foo(); }
  const char* foo() const { return c_->foo(); }

 private:
  std::unique_ptr<Component> c_;
};

class IndirectValueComposite {
 public:
  IndirectValueComposite() : c_(new Component()) {}
  const char* foo() { return c_->foo(); }
  const char* foo() const { return c_->foo(); }

 private:
  indirect_value<Component> c_;
};

int main(int argc, const char** argv) {
  UniquePtrComposite upc;
  std::cout << upc.foo() << std::endl;                 // prints "non-const-foo"
  std::cout << std::as_const(upc).foo() << std::endl;  // prints "non-const-foo"

  IndirectValueComposite ivc;
  std::cout << ivc.foo() << std::endl;                 // prints "non-const-foo"
  std::cout << std::as_const(ivc).foo() << std::endl;  // prints "const-foo"
}
```


Using `indirect_value` a pimpl class can be written as:

~~~ {.cpp}
// header.h
#include <indirect_value.h>

class interface_type {
public:
    interface_type();
    interface_type(example_pimpl&& rhs) noexcept;
    interface_type(const example_pimpl& rhs);
    interface_type& operator=(interface_type&& rhs) noexcept;
    interface_type& operator=(const interface_type& rhs);
    ~interface_type();

private:
    isocpp_p1950::indirect_value<class pimpl> pimpl_;
};
~~~

~~~ {.cpp}
// source.cc
class pimpl {
    // Internal implementation details for the Pimpl
    ...
};

// Force instantiation of the rule of zero methods in the translation unit where implementation details are known.
interface_type::interface_type(interface_type&& rhs) noexcept = default;
interface_type::interface_type(const interface_type& rhs) = default;
interface_type& interface_type::operator=(interface_type&& rhs) noexcept = default;
interface_type& interface_type::operator=(const interface_type& rhs) = default;
interface_type::~interface_type() = default;
~~~

# ISO Standardisation
[`indirect_value` has been proposed for standardisation for C++23 in P1950R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1950r1.html)

# Contents
- [Integration](#integration)
  - [CMake](#cmake)
    - [External](#external)
- [Building](#building)
  - [Building Manually Via CMake](#building-manually-via-cmake)
  - [Installing Via CMake](#installing-via-cmake)
- [Packaging](#packaging)
  - [Conan](#conan)

# Integration
Indirect value is shipped as a single header file, [`indirect_value.h`](https://github.com/jbcoe/indirect_value/blob/master/indirect_value.h) that can be directly included in your project or included via an official [release package](https://github.com/jbcoe/polymorphic_value/releases).

## CMake
To include in your CMake build then add a dependency upon the interface target, `indirect_value::indirect_value`.  This provides the necessary include paths and C++ features required to include `polymorphic_value` into your project.

### External
To include `indirect_value` you will need use find package to locate the provided namespace imported targets from the generated package configuration.  The package configuration file, *polymorphic_value-config.cmake* can be included from the install location or directly out of the build tree.
```cmake
# CMakeLists.txt
find_package(indirect_value 1.0.0 REQUIRED)
...
add_library(foo ...)
...
target_link_libraries(foo PRIVATE indirect_value::indirect_value)
```
# Building

The project contains a helper scripts for building that can be found at **<project root>/scripts/build.py**. The project can be build with the helper script as follows:

```bash
cd <project root>
python script/build.py [--clean] [-o|--output=<build dir>] [-c|--config=<Debug|Release>] [--sanitizers] [-v|--verbose] [-t|--tests]
```

The script will by default build the project via Visual Studio on Windows. On Linux and Mac it will attempt to build via Ninja if available, then Make and will default to the system defaults for choice of compiler.

## Building Manually Via CMake

It is possible to build the project manually via CMake for a finer grained level of control regarding underlying build systems and compilers. This can be achieved as follows:
```bash
cd <project root>
mkdir build
cd build

cmake -G <generator> <configuration options> ../
cmake --build ../
ctest
```

The following configuration options are available:

| Name                    | Possible Values | Description                             | Default Value                  |
|-------------------------|-----------------|-----------------------------------------|--------------------------------|
| `BUILD_TESTING`         | `ON`, `OFF`     | Build the test suite                    | `ON`                           |
| `ENABLE_SANITIZERS`     | `ON`, `OFF`     | Build the tests with sanitizers enabled | `OFF`                          |
| `ENABLE_INCLUDE_NATVIS` | `ON`, `OFF`     | Include natvis file in builds           | `ON` (for MSVC) else `OFF`     |
| `Catch2_ROOT`           | `<path>`        | Path to a Catch2 installation           | undefined                      |


## Installing Via CMake

```bash
cd <project root>
mkdir build
cd build

cmake -G <generator> <configuration options> -DCMAKE_INSTALL_PREFIX=<install dir> ../
cmake --install ../
```

# Packaging

The Indirect Value library is available for integration into your own project via our favorite package manager: [Conan](https://docs.conan.io/en/latest/).

## Conan

Polymorphic Value is now available on the Conan Center Index: https://conan.io/center/indirect_value.  Just include the following dependency in your `conanfile.txt` or `conanfile.py` within your project, install via Conan and build using build system of choice.

```bash
indirect_value/0.0.1
