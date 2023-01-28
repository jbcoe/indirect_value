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

#ifndef INDIRECT_VALUE_EXAMPLE_PIMPL_H
#define INDIRECT_VALUE_EXAMPLE_PIMPL_H

#include "indirect.h"

class example_pimpl {
 public:
  example_pimpl();
  example_pimpl(char const* const name);
  example_pimpl(example_pimpl&& rhs) noexcept;
  example_pimpl(const example_pimpl& rhs);
  example_pimpl& operator=(example_pimpl&& rhs) noexcept;
  example_pimpl& operator=(const example_pimpl& rhs);
  ~example_pimpl();

  const bool is_valid() const noexcept { return static_cast<bool>(pimpl_); }

  // Abstract string representation for ABI safety.
  char const* const get_name() const noexcept;
  void set_name(char const* const name);

 private:
  isocpp_p1950::indirect<class pimpl> pimpl_;
};

#endif  // INDIRECT_VALUE_EXAMPLE_PIMPL_H
