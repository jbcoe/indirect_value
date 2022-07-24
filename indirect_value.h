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

#ifndef ISOCPP_P1950_INDIRECT_VALUE_H
#define ISOCPP_P1950_INDIRECT_VALUE_H

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
#include <compare>
#endif

#ifdef __cpp_concepts
#define ISOCPP_P1950_REQUIRES(x) requires x
#else
#define ISOCPP_P1950_REQUIRES(x)
#endif

// MSVC does not apply EBCO for more than one base class, by default. To enable
// it, you have to write `__declspec(empty_bases)` to the declaration of the
// derived class. As indirect_value inherits from two EBCO - classes, one for
// the copier and one for the deleter, we define a macro to inject that
// __declspec under MSVC.
#ifdef _MSC_VER
#define ISOCPP_P1950_EMPTY_BASES __declspec(empty_bases)
#else
#define ISOCPP_P1950_EMPTY_BASES
#endif

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  using deleter_type = std::default_delete<T>;
  T* operator()(const T& t) const { return new T(t); }
};

template <class T, class = void>
struct copier_traits_deleter_base {};

template <class T>
struct copier_traits_deleter_base<T, std::void_t<typename T::deleter_type>> {
  using deleter_type = typename T::deleter_type;
};

template <class U, class V>
struct copier_traits_deleter_base<U* (*)(V)> {
  using deleter_type = void (*)(U*);
};

// The user may specialize copier_traits<T> per [namespace.std]/2.
template <class T>
struct copier_traits : copier_traits_deleter_base<T, void> {};

class bad_indirect_value_access : public std::exception {
 public:
  const char* what() const noexcept override {
    return "bad_indirect_value_access";
  }
};

template <typename T, typename = void>
constexpr bool is_complete_v = false;
template <typename T>
constexpr bool is_complete_v<T, std::enable_if_t<sizeof(T)>> = true;

template <class C,
          bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C>>
class indirect_value_copy_base {
 protected:
  indirect_value_copy_base() = default;
  indirect_value_copy_base(const C& c) : c_(c) {}
  indirect_value_copy_base(C&& c) : c_(std::move(c)) {}
  C& get() noexcept { return c_; }
  const C& get() const noexcept { return c_; }
  C c_;
};

template <class C>
class indirect_value_copy_base<C, true> : private C {
 protected:
  indirect_value_copy_base() = default;
  indirect_value_copy_base(const C& c) : C(c) {}
  indirect_value_copy_base(C&& c) : C(std::move(c)) {}
  C& get() noexcept { return *this; }
  const C& get() const noexcept { return *this; }
};

template <class D,
          bool CanBeEmptyBaseClass = std::is_empty_v<D> && !std::is_final_v<D>>
class indirect_value_delete_base {
 protected:
  indirect_value_delete_base() = default;
  indirect_value_delete_base(const D& d) : d_(d) {}
  indirect_value_delete_base(D&& d) : d_(std::move(d)) {}
  D& get() noexcept { return d_; }
  const D& get() const noexcept { return d_; }
  D d_;
};

template <class D>
class indirect_value_delete_base<D, true> : private D {
 protected:
  indirect_value_delete_base() = default;
  indirect_value_delete_base(const D& d) : D(d) {}
  indirect_value_delete_base(D&& d) : D(std::move(d)) {}
  D& get() noexcept { return *this; }
  const D& get() const noexcept { return *this; }
};

template <class T, class C = default_copy<T>,
          class D = typename copier_traits<C>::deleter_type>
class ISOCPP_P1950_EMPTY_BASES indirect_value
    : private indirect_value_copy_base<C>,
      private indirect_value_delete_base<D> {
  using copy_base = indirect_value_copy_base<C>;
  using delete_base = indirect_value_delete_base<D>;

  T* ptr_ = nullptr;

  indirect_value() = default;

 public:
  using value_type = T;
  using copier_type = C;
  using deleter_type = D;

  template <class... Ts>
  explicit indirect_value(std::in_place_t, Ts&&... ts)
      : ptr_(new T(std::forward<Ts>(ts)...)) {}

  template <class U,
            class = std::enable_if_t<
                std::is_same_v<T, U> && std::is_default_constructible_v<C> &&
                not std::is_pointer_v<C> &&
                std::is_default_constructible_v<D> && not std::is_pointer_v<D>>>
  explicit indirect_value(U* u) noexcept
      : copy_base(C{}), delete_base(D{}), ptr_(u) {}

  template <class U,
            class = std::enable_if_t<std::is_same_v<T, U> &&
                                     std::is_default_constructible_v<D> &&
                                     not std::is_pointer_v<D>>>
  explicit indirect_value(U* u, C c) noexcept
      : copy_base(std::move(c)), delete_base(D{}), ptr_(u) {}

  template <class U, class = std::enable_if_t<std::is_same_v<T, U>>>
  explicit indirect_value(U* u, C c, D d) noexcept
      : copy_base(std::move(c)), delete_base(std::move(d)), ptr_(u) {}

  indirect_value(const indirect_value& i)
      ISOCPP_P1950_REQUIRES((!is_complete_v<T>) ||
                            std::is_copy_constructible_v<T>)
      : copy_base(i.get_c()), delete_base(i.get_d()), ptr_(i.make_raw_copy()) {}

  indirect_value(indirect_value&& i) noexcept
      : copy_base(std::move(i)),
        delete_base(std::move(i)),
        ptr_(std::exchange(i.ptr_, nullptr)) {}

  indirect_value& operator=(const indirect_value& i)
      ISOCPP_P1950_REQUIRES((!is_complete_v<T>) ||
                            std::is_copy_constructible_v<T>) {
    // When copying T throws, *this will remain unchanged.
    // When assigning copy_base or delete_base throws,
    // ptr_ will be null.
    auto temp_guard = i.make_guarded_copy();
    reset();
    copy_base::operator=(i);
    delete_base::operator=(i);
    ptr_ = temp_guard.release();
    return *this;
  }

  indirect_value& operator=(indirect_value&& i) noexcept {
    if (this != &i) {
      reset();
      copy_base::operator=(std::move(i));
      delete_base::operator=(std::move(i));
      ptr_ = std::exchange(i.ptr_, nullptr);
    }
    return *this;
  }

  ~indirect_value() { reset(); }

  T* operator->() noexcept {
    assert(!valueless_after_move());
    return ptr_;
  }

  const T* operator->() const noexcept {
    assert(!valueless_after_move());
    return ptr_;
  }

  T& operator*() & noexcept {
    assert(!valueless_after_move());
    return *ptr_;
  }

  const T& operator*() const& noexcept {
    assert(!valueless_after_move());
    return *ptr_;
  }

  T&& operator*() && noexcept {
    assert(!valueless_after_move());
    return std::move(*ptr_);
  }

  const T&& operator*() const&& noexcept {
    assert(!valueless_after_move());
    return std::move(*ptr_);
  }

  bool valueless_after_move() const { return ptr_ == nullptr; }

  copier_type& get_copier() noexcept { return get_c(); }

  const copier_type& get_copier() const noexcept { return get_c(); }

  deleter_type& get_deleter() noexcept { return get_d(); }

  const deleter_type& get_deleter() const noexcept { return get_d(); }

  void swap(indirect_value& rhs) noexcept(
      std::is_nothrow_swappable_v<C>&& std::is_nothrow_swappable_v<D>) {
    using std::swap;
    swap(get_c(), rhs.get_c());
    swap(get_d(), rhs.get_d());
    swap(ptr_, rhs.ptr_);
  }

  template <class TC = C>
  friend std::enable_if_t<std::is_swappable_v<TC> && std::is_swappable_v<D>>
  swap(indirect_value& lhs,
       indirect_value& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  C& get_c() noexcept { return copy_base::get(); }
  const C& get_c() const noexcept { return copy_base::get(); }
  D& get_d() noexcept { return delete_base::get(); }
  const D& get_d() const noexcept { return delete_base::get(); }

  void reset() noexcept {
    if (ptr_) {
      // Make sure to first set ptr_ to nullptr before calling the deleter.
      // This will protect us in case that the deleter invokes some code
      // which again accesses ptr_.
      get_d()(std::exchange(ptr_, nullptr));
    }
  }

  T* make_raw_copy() const { return ptr_ ? get_c()(*ptr_) : nullptr; }

  std::unique_ptr<T, std::reference_wrapper<const D>> make_guarded_copy()
      const {
    T* copiedPtr = make_raw_copy();
    // Implies that D::operator() must be const qualified.
    auto deleterRef = std::ref(get_d());
    return {copiedPtr, deleterRef};
  }
};

template <class T>
indirect_value(T*) -> indirect_value<T>;

// Relational operators between two indirect_values.
template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator==(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  const bool leftHasValue = bool(lhs);
  return leftHasValue == bool(rhs) && (!leftHasValue || *lhs == *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator!=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  const bool leftHasValue = bool(lhs);
  return leftHasValue != bool(rhs) || (leftHasValue && *lhs != *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator<(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return bool(rhs) && (!bool(lhs) || *lhs < *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator>(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return bool(lhs) && (!bool(rhs) || *lhs > *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator<=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return !bool(lhs) || (bool(rhs) && *lhs <= *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator>=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return !bool(rhs) || (bool(lhs) && *lhs >= *rhs);
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template <class T1, class C1, class D1, std::three_way_comparable_with<T1> T2,
          class C2, class D2>
std::compare_three_way_result_t<T1, T2> operator<=>(
    const indirect_value<T1, C1, D1>& lhs,
    const indirect_value<T2, C2, D2>& rhs) {
  if (lhs && rhs) {
    return *lhs <=> *rhs;
  }
  return bool(lhs) <=> bool(rhs);
}
#endif

// Comparisons with nullptr_t.
template <class T, class C, class D>
bool operator==(const indirect_value<T, C, D>& lhs, std::nullptr_t) noexcept {
  return !lhs;
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template <class T, class C, class D>
std::strong_ordering operator<=>(const indirect_value<T, C, D>& lhs,
                                 std::nullptr_t) {
  return bool(lhs) <=> false;
}
#else
template <class T, class C, class D>
bool operator==(std::nullptr_t, const indirect_value<T, C, D>& rhs) noexcept {
  return !rhs;
}

template <class T, class C, class D>
bool operator!=(const indirect_value<T, C, D>& lhs, std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class C, class D>
bool operator!=(std::nullptr_t, const indirect_value<T, C, D>& rhs) noexcept {
  return bool(rhs);
}

template <class T, class C, class D>
bool operator<(const indirect_value<T, C, D>&, std::nullptr_t) noexcept {
  return false;
}

template <class T, class C, class D>
bool operator<(std::nullptr_t, const indirect_value<T, C, D>& rhs) noexcept {
  return bool(rhs);
}

template <class T, class C, class D>
bool operator>(const indirect_value<T, C, D>& lhs, std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class C, class D>
bool operator>(std::nullptr_t, const indirect_value<T, C, D>&) noexcept {
  return false;
}

template <class T, class C, class D>
bool operator<=(const indirect_value<T, C, D>& lhs, std::nullptr_t) noexcept {
  return !lhs;
}

template <class T, class C, class D>
bool operator<=(std::nullptr_t, const indirect_value<T, C, D>&) noexcept {
  return true;
}

template <class T, class C, class D>
bool operator>=(const indirect_value<T, C, D>&, std::nullptr_t) noexcept {
  return true;
}

template <class T, class C, class D>
bool operator>=(std::nullptr_t, const indirect_value<T, C, D>& rhs) noexcept {
  return !rhs;
}
#endif

// Comparisons with T
template <class T>
using _enable_if_convertible_to_bool =
    std::enable_if_t<std::is_convertible_v<T, bool>, bool>;

template <class LHS, class RHS>
using _enable_if_comparable_with_equal =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() ==
                                            std::declval<const RHS&>())>;

template <class LHS, class RHS>
using _enable_if_comparable_with_not_equal =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() !=
                                            std::declval<const RHS&>())>;

template <class LHS, class RHS>
using _enable_if_comparable_with_less =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() <
                                            std::declval<const RHS&>())>;

template <class LHS, class RHS>
using _enable_if_comparable_with_greater =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() >
                                            std::declval<const RHS&>())>;

template <class LHS, class RHS>
using _enable_if_comparable_with_less_equal =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() <=
                                            std::declval<const RHS&>())>;

template <class LHS, class RHS>
using _enable_if_comparable_with_greater_equal =
    _enable_if_convertible_to_bool<decltype(std::declval<const LHS&>() >=
                                            std::declval<const RHS&>())>;

template <class T, class C, class D, class U>
auto operator==(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_equal<T, U> {
  return lhs && *lhs == rhs;
}

template <class T, class C, class D, class U>
auto operator==(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_equal<T, U> {
  return rhs && lhs == *rhs;
}

template <class T, class C, class D, class U>
auto operator!=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return !lhs || *lhs != rhs;
}

template <class T, class C, class D, class U>
auto operator!=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return !rhs || lhs != *rhs;
}

template <class T, class C, class D, class U>
auto operator<(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return !lhs || *lhs < rhs;
}

template <class T, class C, class D, class U>
auto operator<(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return rhs && lhs < *rhs;
}

template <class T, class C, class D, class U>
auto operator>(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return lhs && *lhs > rhs;
}

template <class T, class C, class D, class U>
auto operator>(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return !rhs || lhs > *rhs;
}

template <class T, class C, class D, class U>
auto operator<=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return !lhs || *lhs <= rhs;
}

template <class T, class C, class D, class U>
auto operator<=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return rhs && lhs <= *rhs;
}

template <class T, class C, class D, class U>
auto operator>=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater_equal<T, U> {
  return lhs && *lhs >= rhs;
}

template <class T, class C, class D, class U>
auto operator>=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_greater_equal<T, U> {
  return !rhs || lhs >= *rhs;
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)

template <class>
inline constexpr bool _is_indirect_value_v = false;

template <class T, class C, class D>
inline constexpr bool _is_indirect_value_v<indirect_value<T, C, D>> = true;

template <class T, class C, class D, class U>
requires(!_is_indirect_value_v<U>) &&
    std::three_way_comparable_with<T, U> std::compare_three_way_result_t<T, U>
    operator<=>(const indirect_value<T, C, D>& lhs, const U& rhs) {
  return bool(lhs) ? *lhs <=> rhs : std::strong_ordering::less;
}
#endif

template <class IndirectValue, bool Enabled>
struct _conditionally_enabled_hash {
  using VTHash = std::hash<typename IndirectValue::value_type>;

  std::size_t operator()(const IndirectValue& key) const
      noexcept(noexcept(VTHash{}(*key))) {
    return key ? VTHash{}(*key) : 0;
  }
};

template <class T>
struct _conditionally_enabled_hash<T,
                                   false> {  // conditionally disabled hash
                                             // base
  _conditionally_enabled_hash() = delete;
  _conditionally_enabled_hash(const _conditionally_enabled_hash&) = delete;
  _conditionally_enabled_hash& operator=(const _conditionally_enabled_hash&) =
      delete;
};

}  // namespace isocpp_p1950

namespace std {
template <class T, class C, class D>
struct hash<::isocpp_p1950::indirect_value<T, C, D>>
    : ::isocpp_p1950::_conditionally_enabled_hash<
          ::isocpp_p1950::indirect_value<T, C, D>,
          is_default_constructible_v<hash<T>>> {};
}  // namespace std

#endif  // ISOCPP_P1950_INDIRECT_VALUE_H
