# An indirect value-type for C++ (perfect-pimpl)

[![conan][badge.conan]][conan]
[![codecov][badge.codecov]][codecov]
[![language][badge.language]][language]
[![license][badge.license]][license]
[![issues][badge.issues]][issues]

[badge.conan]: https://img.shields.io/github/workflow/status/jbcoe/indirect_value/Conan/main?label=Conan
[badge.codecov]: https://img.shields.io/codecov/c/github/jbcoe/indirect_value/master.svg?logo=codecov
[badge.language]: https://img.shields.io/badge/language-C%2B%2B17-yellow.svg
[badge.license]: https://img.shields.io/badge/license-MIT-blue.svg
[badge.issues]: https://img.shields.io/github/issues/jbcoe/indirect_value.svg

[conan]: https://github.com/jbcoe/indirect_value/actions?query=workflow%3A%22Conan%22+branch%3Amain
[codecov]: https://codecov.io/gh/jbcoe/indirect_value
[language]: https://en.wikipedia.org/wiki/C%2B%2B17
[license]: https://en.wikipedia.org/wiki/MIT_License
[issues]: http://github.com/jbcoe/indirect_value/issues

The class template `indirect_value` is proposed for addition to the C++ Standard Library.

The class template, `indirect_value`, confers value-like semantics on a free-store
allocated object.  A `indirect_value<T>` may hold an object of class T, copying
the indirect_value<T> will copy the object of T, and propagation of constness will
propagate from the owning classes to the indirect_value type.

Using `indirect_value` a pimpl class can be written as:

~~~ {.cpp}
//---------------  Header file
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

//--------------- Source file
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
- [Packaging](#packaging)
  - [Conan](#conan)
    - [Building Conan Packages](#building-conan-packages)
- [License](#license)

# Integration
Indirect value is shipped as a single header file, [`indirect_value.h`](https://github.com/jbcoe/indirect_value/blob/master/indirect_value.h) that can be directly included in your project or included via an official [release package](https://github.com/jbcoe/polymorphic_value/releases).

## CMake
To include in your CMake build then add a dependency upon the interface target, `indirect_value::indirect_value`.  This provides the necessary include paths and C++ features required to include `polymorphic_value` into your project.

### Extenal
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