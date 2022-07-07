#ifndef INDIRECT_VALUE_EXAMPLE_PIMPL_H
#define INDIRECT_VALUE_EXAMPLE_PIMPL_H

#include "indirect_value.h"

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
  isocpp_p1950::indirect_value<class pimpl> pimpl_;
};

#endif  // INDIRECT_VALUE_EXAMPLE_PIMPL_H
