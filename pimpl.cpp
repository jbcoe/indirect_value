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

#include <cassert>
#include <string>

class pimpl {
 public:
  const std::string& get_name() const noexcept { return name_; }
  void set_name(const std::string& name) { name_ = name; }

 private:
  std::string name_;
};

example_pimpl::example_pimpl() : pimpl_(std::in_place) {}

example_pimpl::example_pimpl(char const* const name) : pimpl_(std::in_place) {
  assert(name);
  pimpl_->set_name(name);
}

char const* const example_pimpl::get_name() const noexcept {
  assert(!pimpl_->get_name().empty());
  return pimpl_->get_name().c_str();
}

void example_pimpl::set_name(char const* const name) {
  assert(name);
  pimpl_->set_name(name);
}

example_pimpl::example_pimpl(example_pimpl&& rhs) noexcept = default;
example_pimpl::example_pimpl(const example_pimpl& rhs) = default;
example_pimpl& example_pimpl::operator=(example_pimpl&& rhs) noexcept = default;
example_pimpl& example_pimpl::operator=(const example_pimpl& rhs) = default;
example_pimpl::~example_pimpl() = default;