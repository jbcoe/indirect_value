#!/usr/bin/env python
# -*- coding: utf-8 -*-
from conans import ConanFile, CMake

class IndirectValueConan(ConanFile):
    name = "indirect_value"
    version = "0.0.1"
    license = "MIT"
    url = "https://github.com/jbcoe/indirect_value"
    description = "An indirect value-type (perfect-pimpl) proposed for inclusion to the C++ 23 Standard Library"
    topics = ("conan", "indirect value", "header-only", "std", "experimental", "wg21")
    exports_sources = "*.txt", "*.h", "*.natvis", "*.cpp", "*.cmake", "*.cmake.in", "LICENSE.txt"
    generators = "cmake"

    _cmake = None
    @property
    def cmake(self):
        if self._cmake is None:
            self._cmake = CMake(self)
            self._cmake.definitions.update({
                "BUILD_TESTING": False
            })
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.cmake.build()
        if self._cmake.definitions["BUILD_TESTING"]:
            self.cmake.test()

    def package(self):
        self.cmake.install()