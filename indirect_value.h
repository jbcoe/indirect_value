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

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

class bad_indirect_value_access : public std::exception {
 public:
  const char* what() const noexcept override {
    return "bad_indirect_value_access";
  }
};

template <class T, class C = default_copy<T>,
          bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C>>
class indirect_value_base {
 protected:
  template <class U = C,
            class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_value_base() noexcept(noexcept(C())) {}
  indirect_value_base(C c) : c_(std::move(c)) {}
  C& get() noexcept { return c_; }
  const C& get() const noexcept { return c_; }
  C c_;
};

template <class T, class C>
class indirect_value_base<T, C, true> : private C {
 protected:
  template <class U = C,
            class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_value_base() noexcept(noexcept(C())) {}
  indirect_value_base(C c) : C(std::move(c)) {}
  C& get() noexcept { return *this; }
  const C& get() const noexcept { return *this; }
};

template <class T, class C = default_copy<T>, class D = std::default_delete<T>>
class indirect_value : private indirect_value_base<T, C> {
  using base = indirect_value_base<T, C>;

  std::unique_ptr<T, D> ptr_;

 public:
  using value_type = T;
  using copier_type = C;
  using deleter_type = D;

  indirect_value() = default;

  template <class... Ts>
  explicit indirect_value(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  template <class U, class = std::enable_if_t<std::is_same_v<T, U>>>
  explicit indirect_value(U* u, C c = C{}, D d = D{}) noexcept
      : base(std::move(c)), ptr_(std::unique_ptr<T, D>(u, std::move(d))) {}

  indirect_value(const indirect_value& i)
      ISOCPP_P1950_REQUIRES(std::is_copy_constructible_v<T>)
      : base(i.get_c()) {
    if (i.ptr_) {
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), i.ptr_.get_deleter());
    }
  }

  indirect_value(indirect_value&& i) noexcept
      : base(std::move(i)), ptr_(std::exchange(i.ptr_, nullptr)) {}

  indirect_value& operator=(const indirect_value& i)
      ISOCPP_P1950_REQUIRES(std::is_copy_assignable_v<T>) {
    base::operator=(i);
    if (i.ptr_) {
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), i.ptr_.get_deleter());
    } else {
      ptr_.reset();
    }
    return *this;
  }

  indirect_value& operator=(indirect_value&& i) noexcept {
    base::operator=(std::move(i));
    ptr_ = std::exchange(i.ptr_, nullptr);
    return *this;
  }

  ~indirect_value() = default;

  T* operator->() noexcept { return ptr_.operator->(); }

  const T* operator->() const noexcept { return ptr_.operator->(); }

  T& operator*() & noexcept { return *ptr_; }

  const T& operator*() const& noexcept { return *ptr_; }

  T&& operator*() && noexcept { return std::move(*ptr_); }

  const T&& operator*() const&& noexcept { return std::move(*ptr_); }

  T& value() & {
    if (!ptr_) throw bad_indirect_value_access();
    return *ptr_;
  }

  const T& value() const& {
    if (!ptr_) throw bad_indirect_value_access();
    return *ptr_;
  }

  T&& value() && {
    if (!ptr_) throw bad_indirect_value_access();
    return std::move(*ptr_);
  }

  const T&& value() const&& {
    if (!ptr_) throw bad_indirect_value_access();
    return std::move(*ptr_);
  }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  bool has_value() const noexcept { return ptr_ != nullptr; }

  copier_type& get_copier() noexcept { return get_c(); }

  const copier_type& get_copier() const noexcept { return get_c(); }

  deleter_type& get_deleter() noexcept { return ptr_.get_deleter(); }

  const deleter_type& get_deleter() const noexcept {
    return ptr_.get_deleter();
  }

  void swap(indirect_value& rhs) noexcept(
      std::is_nothrow_swappable_v<C>&& std::is_nothrow_swappable_v<D>) {
    using std::swap;
    swap(get_c(), rhs.get_c());
    swap(ptr_, rhs.ptr_);
  }

  template <class TC = C>
  friend std::enable_if_t<std::is_swappable_v<TC> && std::is_swappable_v<D>>
  swap(indirect_value& lhs,
       indirect_value& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  C& get_c() noexcept { return base::get(); }
  const C& get_c() const noexcept { return base::get(); }
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
                                   false> {  // conditionally disabled hash base
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
