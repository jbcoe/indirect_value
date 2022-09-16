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

#include <cassert>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

#if __has_include(<optional>)
#include <optional>
#endif

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

#if (__cpp_lib_optional >= 201606)
  friend class std::optional<::isocpp_p1950::indirect_value<T, C, D>>;

  constexpr indirect_value(std::nullptr_t) noexcept : ptr_(nullptr) {}

#endif  // #if (__cpp_lib_optional > 201606)

 public:
  using value_type = T;
  using copier_type = C;
  using deleter_type = D;

  template <class U = T, std::enable_if_t<std::is_default_constructible_v<U>,
                                          void*> = nullptr>
  constexpr indirect_value() : ptr_(new T()) {}

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

  bool valueless_after_move() const noexcept { return ptr_ == nullptr; }

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
  const bool leftHasValue = !lhs.valueless_after_move();
  return leftHasValue == !rhs.valueless_after_move() &&
         (!leftHasValue || *lhs == *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator!=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  const bool leftHasValue = !lhs.valueless_after_move();
  return leftHasValue != !rhs.valueless_after_move() ||
         (leftHasValue && *lhs != *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator<(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return !rhs.valueless_after_move() &&
         (lhs.valueless_after_move() || *lhs < *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator>(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return !lhs.valueless_after_move() &&
         (rhs.valueless_after_move() || *lhs > *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator<=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return lhs.valueless_after_move() ||
         (!rhs.valueless_after_move() && *lhs <= *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
bool operator>=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return rhs.valueless_after_move() ||
         (!lhs.valueless_after_move() && *lhs >= *rhs);
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template <class T1, class C1, class D1, std::three_way_comparable_with<T1> T2,
          class C2, class D2>
std::compare_three_way_result_t<T1, T2> operator<=>(
    const indirect_value<T1, C1, D1>& lhs,
    const indirect_value<T2, C2, D2>& rhs) {
  if (!lhs.valueless_after_move() && !rhs.valueless_after_move()) {
    return *lhs <=> *rhs;
  }
  return !lhs.valueless_after_move() <=> !rhs.valueless_after_move();
}
#endif

// Comparisons with nullptr_t.
template <class T, class C, class D>
bool operator==(const indirect_value<T, C, D>& lhs, std::nullptr_t) noexcept {
  return lhs.valueless_after_move();
}

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
  return !lhs.valueless_after_move() && *lhs == rhs;
}

template <class T, class C, class D, class U>
auto operator==(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_equal<T, U> {
  return !rhs.valueless_after_move() && lhs == *rhs;
}

template <class T, class C, class D, class U>
auto operator!=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return lhs.valueless_after_move() || *lhs != rhs;
}

template <class T, class C, class D, class U>
auto operator!=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return rhs.valueless_after_move() || lhs != *rhs;
}

template <class T, class C, class D, class U>
auto operator<(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return lhs.valueless_after_move() || *lhs < rhs;
}

template <class T, class C, class D, class U>
auto operator<(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return !rhs.valueless_after_move() && lhs < *rhs;
}

template <class T, class C, class D, class U>
auto operator>(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return !lhs.valueless_after_move() && *lhs > rhs;
}

template <class T, class C, class D, class U>
auto operator>(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return rhs.valueless_after_move() || lhs > *rhs;
}

template <class T, class C, class D, class U>
auto operator<=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return lhs.valueless_after_move() || *lhs <= rhs;
}

template <class T, class C, class D, class U>
auto operator<=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return !rhs.valueless_after_move() && lhs <= *rhs;
}

template <class T, class C, class D, class U>
auto operator>=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater_equal<T, U> {
  return !lhs.valueless_after_move() && *lhs >= rhs;
}

template <class T, class C, class D, class U>
auto operator>=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_greater_equal<T, U> {
  return rhs.valueless_after_move() || lhs >= *rhs;
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
  return !lhs.valueless_after_move() ? *lhs <=> rhs
                                     : std::strong_ordering::less;
}
#endif

template <class IndirectValue, bool Enabled>
struct _conditionally_enabled_hash {
  using VTHash = std::hash<typename IndirectValue::value_type>;

  std::size_t operator()(const IndirectValue& key) const
      noexcept(noexcept(VTHash{}(*key))) {
    assert(!key.valueless_after_move());
    return VTHash{}(*key);
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

template <class T, class C, class D>
struct std::hash<::isocpp_p1950::indirect_value<T, C, D>>
    : ::isocpp_p1950::_conditionally_enabled_hash<
          ::isocpp_p1950::indirect_value<T, C, D>,
          is_default_constructible_v<hash<T>>> {};

#if (__cpp_lib_optional >= 201606)

#if (__cpp_lib_optional >= 202106)
#define OPTIONAL_CONSTEXPR constexpr
#else
#define OPTIONAL_CONSTEXPR
#endif  // #if (__cpp_lib_optional > 202106)

template <class T, class C, class D>
class std::optional<::isocpp_p1950::indirect_value<T, C, D>> {
  template <typename... condition>
  using enable_if_all_of_t =
      enable_if_t<std::conjunction<condition...>::value, void*>;

  template <typename U>
  using not_same = std::negation<
      std::is_same<optional, std::remove_cv<std::remove_reference<U>>>>;

  template <typename U>
  using not_in_place =
      std::negation<std::is_same<std::in_place_t,
                                 std::remove_cv<std::remove_reference<U>>>>;

  template <typename U>
  using constructible_from = std::is_constructible<T, U>;

  template <typename U>
  using convertible_to = std::is_convertible<U, T>;

  template <typename U>
  using convertible_from_optional =
      std::disjunction<std::is_constructible<T, const optional<U>&>,
                       std::is_constructible<T, optional<U>&>,
                       std::is_constructible<T, const optional<U>&&>,
                       std::is_constructible<T, optional<U>&&>,
                       std::is_convertible<const optional<U>&, T>,
                       std::is_convertible<optional<U>&, T>,
                       std::is_convertible<const optional<U>&&, T>,
                       std::is_convertible<optional<U>&&, T>>;

  template <typename U>
  using assignable_from_optional =
      std::disjunction<std::is_assignable<T&, const optional<U>&>,
                       std::is_assignable<T&, optional<U>&>,
                       std::is_assignable<T&, const optional<U>&&>,
                       std::is_assignable<T&, optional<U>&&>>;

  [[noreturn]] inline void throw_bad_optional_access() const {
    throw std::bad_optional_access();
  }

 public:
  using value_type = ::isocpp_p1950::indirect_value<T, C, D>;

  constexpr optional() noexcept : mIndirectValue(nullptr) {}
  constexpr optional(std::nullopt_t) noexcept : mIndirectValue(nullptr) {}
  constexpr optional(const optional& other) = default;
  constexpr optional(optional&& other) noexcept(
      noexcept(is_nothrow_constructible_v<T, T>)) = default;

#if (__cpp_conditional_explicit >= 201806)

  // 4
  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>> = nullptr>
  OPTIONAL_CONSTEXPR explicit(!std::is_convertible_v<U, T>)
      optional(const optional<U>& other)
      : mIndirectValue((!other) ? nullptr
                                : isocpp_p1950::indirect_value<T, C, D>(
                                      std::in_place, other.value())) {}

  // 5
  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>> = nullptr>
  OPTIONAL_CONSTEXPR explicit(!std::is_convertible_v<U, T>)
      optional(optional<U>&& other)
      : mIndirectValue((!other)
                           ? nullptr
                           : isocpp_p1950::indirect_value<T, C, D>(
                                 std::in_place, std::move(other).value())) {}

  // 8

  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>> = nullptr>
  OPTIONAL_CONSTEXPR explicit(!std::is_convertible_v<U, T>) optional(U&& value)
      : mIndirectValue(in_place, std::forward<U>(value)) {}

#else

  template <
      class U = T,
      enable_if_all_of_t<not_same<U>, not_in_place<U>, constructible_from<U>,
                         std::negation<convertible_to<U>>> = nullptr>
  OPTIONAL_CONSTEXPR explicit optional(const optional<U>& other)
      : mIndirectValue((!other) ? nullptr
                                : isocpp_p1950::indirect_value<T, C, D>(
                                      std::in_place, other.value())) {}

  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>,
                                            convertible_to<U>> = nullptr>
  OPTIONAL_CONSTEXPR optional(const optional<U>& other)
      : mIndirectValue((!other) ? nullptr
                                : isocpp_p1950::indirect_value<T, C, D>(
                                      std::in_place, other.value())) {}

  template <
      class U = T,
      enable_if_all_of_t<not_same<U>, not_in_place<U>, constructible_from<U>,
                         std::negation<convertible_to<U>>> = nullptr>
  OPTIONAL_CONSTEXPR explicit optional(optional<U>&& other)
      : mIndirectValue((!other)
                           ? nullptr
                           : isocpp_p1950::indirect_value<T, C, D>(
                                 std::in_place, std::move(other).value())) {}

  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>,
                                            convertible_to<U>> = nullptr>
  OPTIONAL_CONSTEXPR optional(optional<U>&& other)
      : mIndirectValue((!other)
                           ? nullptr
                           : isocpp_p1950::indirect_value<T, C, D>(
                                 std::in_place, std::move(other).value())) {}

  template <class U = T, enable_if_all_of_t<not_same<U>, not_in_place<U>,
                                            constructible_from<U>,
                                            convertible_to<U>> = nullptr>
  OPTIONAL_CONSTEXPR optional(U&& value)
      : mIndirectValue(in_place, std::forward<U>(value)) {}

  template <
      class U = T,
      enable_if_all_of_t<not_same<U>, not_in_place<U>, constructible_from<U>,
                         std::negation<convertible_to<U>>> = nullptr>
  OPTIONAL_CONSTEXPR explicit optional(U&& value)
      : mIndirectValue(in_place, std::forward<U>(value)) {}
#endif  // (__cpp_conditional_explicit > 201806)

  template <class... Args, enable_if_t<std::is_constructible<T, Args...>::value,
                                       void*> = nullptr>
  constexpr explicit optional(std::in_place_t, Args&&... args)
      : mIndirectValue(in_place, std::forward<Args>(args)...) {}

  template <class U, class... Args,
            enable_if_t<std::is_constructible<T, std::initializer_list<U>,
                                              Args...>::value,
                        void*> = nullptr>
  constexpr explicit optional(std::in_place_t, std::initializer_list<U> ilist,
                              Args&&... args)
      : mIndirectValue(in_place, ilist, std::forward<Args>(args)...) {}

  OPTIONAL_CONSTEXPR optional& operator=(std::nullopt_t) noexcept {
    mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(nullptr);
    return *this;
  }

  constexpr optional& operator=(const optional& other) = default;
  constexpr optional& operator=(optional&& other) noexcept = default;

  template <class U = T, enable_if_all_of_t<std::negation<std::is_scalar<U>>>,
            not_same<U>, not_in_place<U>, std::is_constructible<T, U>,
            std::is_assignable<T, U> = nullptr>
  OPTIONAL_CONSTEXPR optional& operator=(U&& value) noexcept(
      std::conjunction_v<std::is_nothrow_constructible<T, U>,
                         std::is_nothrow_assignable<T, U>>) {
    mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(
        in_place, std::forward<U>(value));
    return *this;
  }

  template <
      class U = T,
      enable_if_all_of_t<
          std::negation<std::is_same<T, U>>, std::is_constructible<T, const U&>,
          std::is_assignable<T, U>, std::negation<convertible_from_optional<U>>,
          std::negation<assignable_from_optional<U>>> = nullptr>
  OPTIONAL_CONSTEXPR optional& operator=(const optional<U>& other) noexcept(
      std::conjunction_v<std::is_nothrow_constructible<T, U>,
                         std::is_nothrow_assignable<T, U>>) {
    if (!other)
      mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(nullptr);
    else
      mIndirectValue =
          ::isocpp_p1950::indirect_value<T, C, D>(std::in_place, other.value());
    return *this;
  }

  template <
      class U = T,
      enable_if_all_of_t<
          std::negation<std::is_same<T, U>>, std::is_constructible<T, const U&>,
          std::is_assignable<T, U>, std::negation<convertible_from_optional<U>>,
          std::negation<assignable_from_optional<U>>> = nullptr>
  OPTIONAL_CONSTEXPR optional& operator=(optional<U>&& other) noexcept(
      std::conjunction_v<std::is_nothrow_constructible<T, U>,
                         std::is_nothrow_assignable<T, U>>) {
    if (!other)
      mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(nullptr);
    else
      mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(
          std::in_place, std::move(other).value());
    return *this;
  }

  constexpr const T* operator->() const noexcept {
    return mIndirectValue.operator->();
  }
  constexpr T* operator->() noexcept { return mIndirectValue.operator->(); }
  constexpr const T& operator*() const& noexcept {
    return mIndirectValue.operator*();
  }
  constexpr T& operator*() & noexcept { return mIndirectValue.operator*(); }
  constexpr const T&& operator*() const&& noexcept {
    return mIndirectValue.operator*();
  }
  constexpr T&& operator*() && noexcept { return mIndirectValue.operator*(); }

  constexpr explicit operator bool() const noexcept {
    return mIndirectValue.ptr_ != nullptr;
  }
  constexpr bool has_value() const noexcept {
    return mIndirectValue.ptr_ != nullptr;
  }

  constexpr auto& value() & {
    if (has_value())
      return mIndirectValue;
    else
      throw_bad_optional_access();
  }
  constexpr const auto& value() const& {
    if (has_value())
      return mIndirectValue;
    else
      throw_bad_optional_access();
  }
  constexpr auto&& value() && {
    if (has_value())
      return std::move(mIndirectValue);
    else
      throw_bad_optional_access();
  }
  constexpr const auto&& value() const&& {
    if (has_value())
      return std::move(mIndirectValue);
    else
      throw_bad_optional_access();
  }

  template <class U>
  constexpr T value_or(U&& default_value) const& {
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_convertible_v<U&&, T>);

    if (has_value())
      return operator*();
    else
      return static_cast<T>(std::forward<U>(default_value));
  }

  template <class U>
  constexpr T value_or(U&& default_value) && {
    static_assert(std::is_copy_constructible_v<T>);
    static_assert(std::is_convertible_v<U&&, T>);

    if (has_value())
      return operator*();
    else
      return static_cast<T>(std::forward<U>(default_value));
  }

  OPTIONAL_CONSTEXPR void reset() noexcept { mIndirectValue.reset(); }

  OPTIONAL_CONSTEXPR void swap(optional& other) { mIndirectValue.swap(); }

  template <class... Args>
  OPTIONAL_CONSTEXPR T& emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    reset();
    mIndirectValue =
        ::isocpp_p1950::indirect_value<T, C, D>(std::forward<Args>(args)...);
    return mIndirectValue.get();
  }

  template <class U, class... Args>
  OPTIONAL_CONSTEXPR T&
  emplace(std::initializer_list<U> ilist, Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, std::initializer_list<U>&, Args...>) {
    reset();
    mIndirectValue = ::isocpp_p1950::indirect_value<T, C, D>(
        ilist, std::forward<Args>(args)...);
    return mIndirectValue.get();
  }

 private:
  value_type mIndirectValue;
};

#endif  // #if (__cpp_lib_optional > 201606)

#endif  // ISOCPP_P1950_INDIRECT_VALUE_H
