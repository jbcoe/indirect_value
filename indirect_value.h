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

#if (__cpp_constexpr >= 202002)
    #define ISOCPP_P1950_CONSTEXPR_CXX20 constexpr
#else
    #define ISOCPP_P1950_CONSTEXPR_CXX20
#endif

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  using deleter_type = std::default_delete<T>;
  constexpr T* operator()(const T& t) const { return new T(t); }
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
struct copier_traits
    : copier_traits_deleter_base<T, void> {
};

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

namespace detail
{

template <typename T, typename A, typename... Args>
ISOCPP_P1950_CONSTEXPR_CXX20 T* allocate_object(A& a, Args&&... args) {
  using t_allocator =
      typename std::allocator_traits<A>::template rebind_alloc<T>;
  using t_traits = std::allocator_traits<t_allocator>;
  t_allocator t_alloc(a);
  T* mem = t_traits::allocate(t_alloc, 1);
  try {
    t_traits::construct(t_alloc, mem, std::forward<Args>(args)...);
    return mem;
  } catch (...) {
    t_traits::deallocate(t_alloc, mem, 1);
    throw;
  }
}

template <typename T, typename A>
constexpr void deallocate_object(A& a, T* p) {
  using t_allocator =
      typename std::allocator_traits<A>::template rebind_alloc<T>;
  using t_traits = std::allocator_traits<t_allocator>;
  t_allocator t_alloc(a);
  t_traits::destroy(t_alloc, p);
  t_traits::deallocate(t_alloc, p, 1);
};

template <class T, class A>
struct allocator_delete : A  {
    constexpr allocator_delete(A& a) : A(a) {} 
    constexpr void operator()(T* ptr) const noexcept { 
        static_assert(0 < sizeof(T), "can't delete an incomplete type");
        detail::deallocate_object(*this, ptr);
    }
};

template <class T, class A>
struct allocator_copy : A {
  constexpr allocator_copy(A& a) : A(a) {} 
  using deleter_type = allocator_delete<T, A>;
  constexpr T* operator()(const T& t) const { 
    return detail::allocate_object<T>(*this, t);
  }
};

}

template <class C,
          bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C>>
class indirect_value_copy_base {
 protected:
  constexpr indirect_value_copy_base() = default;
  constexpr indirect_value_copy_base(const C& c) : c_(c) {}
  constexpr indirect_value_copy_base(C&& c) : c_(std::move(c)) {}
  constexpr C& get() noexcept { return c_; }
  constexpr const C& get() const noexcept { return c_; }
  C c_;
};

template <class C>
class indirect_value_copy_base<C, true> : private C {
 protected:
  constexpr indirect_value_copy_base() = default;
  constexpr indirect_value_copy_base(const C& c) : C(c) {}
  constexpr indirect_value_copy_base(C&& c) : C(std::move(c)) {}
  constexpr C& get() noexcept { return *this; }
  constexpr const C& get() const noexcept { return *this; }
};

template <class D,
          bool CanBeEmptyBaseClass = std::is_empty_v<D> && !std::is_final_v<D>>
class indirect_value_delete_base {
 protected:
  constexpr indirect_value_delete_base() = default;
  constexpr indirect_value_delete_base(const D& d) : d_(d) {}
  constexpr indirect_value_delete_base(D&& d) : d_(std::move(d)) {}
  constexpr D& get() noexcept { return d_; }
  constexpr const D& get() const noexcept { return d_; }
  D d_;
};

template <class D>
class indirect_value_delete_base<D, true> : private D {
 protected:
  constexpr indirect_value_delete_base() = default;
  constexpr indirect_value_delete_base(const D& d) : D(d) {}
  constexpr indirect_value_delete_base(D&& d) : D(std::move(d)) {}
  constexpr D& get() noexcept { return *this; }
  constexpr const D& get() const noexcept { return *this; }
};

template <class T, class C = default_copy<T>, class D = typename copier_traits<C>::deleter_type>
class ISOCPP_P1950_EMPTY_BASES indirect_value
    : private indirect_value_copy_base<C>,
      private indirect_value_delete_base<D> {
  using copy_base = indirect_value_copy_base<C>;
  using delete_base = indirect_value_delete_base<D>;

  T* ptr_ = nullptr;

 public:
  using value_type = T;
  using copier_type = C;
  using deleter_type = D;

  constexpr indirect_value() = default;

  template <class... Ts>
  constexpr explicit indirect_value(std::in_place_t, Ts&&... ts)
      : ptr_(new T(std::forward<Ts>(ts)...)) {}

  template <class U, class = std::enable_if_t<std::is_same_v<T, U> &&
      std::is_default_constructible_v<C> &&
      not std::is_pointer_v<C> &&
      std::is_default_constructible_v<D> &&
      not std::is_pointer_v<D>>>
  constexpr explicit indirect_value(U* u) noexcept
      : copy_base(C{}), delete_base(D{}), ptr_(u) {}

  template <class U, class = std::enable_if_t<std::is_same_v<T, U> &&
      std::is_default_constructible_v<D> &&
      not std::is_pointer_v<D>>>
  constexpr explicit indirect_value(U* u, C c) noexcept
      : copy_base(std::move(c)), delete_base(D{}), ptr_(u) {}

  template <class U, class = std::enable_if_t<std::is_same_v<T, U>>>
  constexpr explicit indirect_value(U* u, C c, D d) noexcept
      : copy_base(std::move(c)), delete_base(std::move(d)), ptr_(u) {}

  constexpr indirect_value(const indirect_value& i)
      ISOCPP_P1950_REQUIRES((!is_complete_v<T>) ||
                            std::is_copy_constructible_v<T>)
      : copy_base(i.get_c()), delete_base(i.get_d()), ptr_(i.make_raw_copy()) {}

  constexpr indirect_value(indirect_value&& i) noexcept
      : copy_base(std::move(i)),
        delete_base(std::move(i)),
        ptr_(std::exchange(i.ptr_, nullptr)) {}

  constexpr indirect_value& operator=(const indirect_value& i)
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

  constexpr indirect_value& operator=(indirect_value&& i) noexcept {
    if (this != &i) {
      reset();
      copy_base::operator=(std::move(i));
      delete_base::operator=(std::move(i));
      ptr_ = std::exchange(i.ptr_, nullptr);
    }
    return *this;
  }

  ISOCPP_P1950_CONSTEXPR_CXX20 ~indirect_value() { reset(); }

  constexpr T* operator->() noexcept { return ptr_; }

  constexpr const T* operator->() const noexcept { return ptr_; }

  constexpr T& operator*() & noexcept { return *ptr_; }

  constexpr const T& operator*() const& noexcept { return *ptr_; }

  constexpr T&& operator*() && noexcept { return std::move(*ptr_); }

  constexpr const T&& operator*() const&& noexcept { return std::move(*ptr_); }

  constexpr T& value() & {
    if (!ptr_) throw bad_indirect_value_access();
    return *ptr_;
  }

  constexpr const T& value() const& {
    if (!ptr_) throw bad_indirect_value_access();
    return *ptr_;
  }

  constexpr T&& value() && {
    if (!ptr_) throw bad_indirect_value_access();
    return std::move(*ptr_);
  }

  constexpr const T&& value() const&& {
    if (!ptr_) throw bad_indirect_value_access();
    return std::move(*ptr_);
  }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  constexpr bool has_value() const noexcept { return ptr_ != nullptr; }

  constexpr copier_type& get_copier() noexcept { return get_c(); }

  constexpr const copier_type& get_copier() const noexcept { return get_c(); }

  constexpr deleter_type& get_deleter() noexcept { return get_d(); }

  constexpr const deleter_type& get_deleter() const noexcept { return get_d(); }

  constexpr void swap(indirect_value& rhs) noexcept(
      std::is_nothrow_swappable_v<C>&& std::is_nothrow_swappable_v<D>) {
    using std::swap;
    swap(get_c(), rhs.get_c());
    swap(get_d(), rhs.get_d());
    swap(ptr_, rhs.ptr_);
  }

  template <class TC = C>
  friend constexpr std::enable_if_t<std::is_swappable_v<TC> &&
                                    std::is_swappable_v<D>>
  swap(indirect_value& lhs,
       indirect_value& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  constexpr C& get_c() noexcept { return copy_base::get(); }
  constexpr const C& get_c() const noexcept { return copy_base::get(); }
  constexpr D& get_d() noexcept { return delete_base::get(); }
  constexpr const D& get_d() const noexcept { return delete_base::get(); }

  constexpr void reset() noexcept {
    if (ptr_) {
      // Make sure to first set ptr_ to nullptr before calling the deleter.
      // This will protect us in case that the deleter invokes some code
      // which again accesses ptr_.
      get_d()(std::exchange(ptr_, nullptr));
    }
  }

  constexpr T* make_raw_copy() const { return ptr_ ? get_c()(*ptr_) : nullptr; }

  constexpr std::unique_ptr<T, std::reference_wrapper<const D>>
  make_guarded_copy()
      const {
    T* copiedPtr = make_raw_copy();
    // Implies that D::operator() must be const qualified.
    auto deleterRef = std::ref(get_d());
    return {copiedPtr, deleterRef};
  }
};

template <class T>
indirect_value(T*) -> indirect_value<T>;

template <class T, class... Ts>
constexpr indirect_value<T> make_indirect_value(Ts&&... ts) {
  return indirect_value<T>(std::in_place_t{}, std::forward<Ts>(ts)...);
}

template <class T, class A = std::allocator<T>, class... Ts>
ISOCPP_P1950_CONSTEXPR_CXX20 auto allocate_indirect_value(std::allocator_arg_t, A& a, Ts&&... ts) {
  auto* u = detail::allocate_object<T>(a, std::forward<Ts>(ts)...);
  try {
    return indirect_value<T, detail::allocator_copy<T, A>, detail::allocator_delete<T, A>>(u, {a}, {a});
  } catch (...) {
    detail::deallocate_object(a, u);
    throw;
  }
}


// Relational operators between two indirect_values.
template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator==(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  const bool leftHasValue = bool(lhs);
  return leftHasValue == bool(rhs) && (!leftHasValue || *lhs == *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator!=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  const bool leftHasValue = bool(lhs);
  return leftHasValue != bool(rhs) || (leftHasValue && *lhs != *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator<(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return bool(rhs) && (!bool(lhs) || *lhs < *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator>(const indirect_value<T1, C1, D1>& lhs,
               const indirect_value<T2, C2, D2>& rhs) {
  return bool(lhs) && (!bool(rhs) || *lhs > *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator<=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return !bool(lhs) || (bool(rhs) && *lhs <= *rhs);
}

template <class T1, class C1, class D1, class T2, class C2, class D2>
constexpr bool operator>=(const indirect_value<T1, C1, D1>& lhs,
                const indirect_value<T2, C2, D2>& rhs) {
  return !bool(rhs) || (bool(lhs) && *lhs >= *rhs);
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template <class T1, class C1, class D1, std::three_way_comparable_with<T1> T2,
          class C2, class D2>
constexpr std::compare_three_way_result_t<T1, T2> operator<=>(
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
constexpr bool operator==(const indirect_value<T, C, D>& lhs,
                          std::nullptr_t) noexcept {
  return !lhs;
}

#if defined(__cpp_lib_three_way_comparison) && defined(__cpp_lib_concepts)
template <class T, class C, class D>
constexpr std::strong_ordering operator<=>(const indirect_value<T, C, D>& lhs,
                                 std::nullptr_t) {
  return bool(lhs) <=> false;
}
#else
template <class T, class C, class D>
constexpr bool operator==(std::nullptr_t,
                          const indirect_value<T, C, D>& rhs) noexcept {
  return !rhs;
}

template <class T, class C, class D>
constexpr bool operator!=(const indirect_value<T, C, D>& lhs,
                          std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class C, class D>
constexpr bool operator!=(std::nullptr_t,
                          const indirect_value<T, C, D>& rhs) noexcept {
  return bool(rhs);
}

template <class T, class C, class D>
constexpr bool operator<(const indirect_value<T, C, D>&,
                         std::nullptr_t) noexcept {
  return false;
}

template <class T, class C, class D>
constexpr bool operator<(std::nullptr_t,
                         const indirect_value<T, C, D>& rhs) noexcept {
  return bool(rhs);
}

template <class T, class C, class D>
constexpr bool operator>(const indirect_value<T, C, D>& lhs,
                         std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class C, class D>
constexpr bool operator>(std::nullptr_t,
                         const indirect_value<T, C, D>&) noexcept {
  return false;
}

template <class T, class C, class D>
constexpr bool operator<=(const indirect_value<T, C, D>& lhs,
                          std::nullptr_t) noexcept {
  return !lhs;
}

template <class T, class C, class D>
constexpr bool operator<=(std::nullptr_t,
                          const indirect_value<T, C, D>&) noexcept {
  return true;
}

template <class T, class C, class D>
constexpr bool operator>=(const indirect_value<T, C, D>&,
                          std::nullptr_t) noexcept {
  return true;
}

template <class T, class C, class D>
constexpr bool operator>=(std::nullptr_t,
                          const indirect_value<T, C, D>& rhs) noexcept {
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
constexpr auto operator==(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_equal<T, U> {
  return lhs && *lhs == rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator==(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_equal<T, U> {
  return rhs && lhs == *rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator!=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return !lhs || *lhs != rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator!=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_not_equal<T, U> {
  return !rhs || lhs != *rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator<(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return !lhs || *lhs < rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator<(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less<T, U> {
  return rhs && lhs < *rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator>(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return lhs && *lhs > rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator>(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_greater<T, U> {
  return !rhs || lhs > *rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator<=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return !lhs || *lhs <= rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator<=(const T& lhs, const indirect_value<U, C, D>& rhs)
    -> _enable_if_comparable_with_less_equal<T, U> {
  return rhs && lhs <= *rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator>=(const indirect_value<T, C, D>& lhs, const U& rhs)
    -> _enable_if_comparable_with_greater_equal<T, U> {
  return lhs && *lhs >= rhs;
}

template <class T, class C, class D, class U>
constexpr auto operator>=(const T& lhs, const indirect_value<U, C, D>& rhs)
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

  constexpr std::size_t operator()(const IndirectValue& key) const
      noexcept(noexcept(VTHash{}(*key))) {
    return key ? VTHash{}(*key) : 0;
  }
};

template <class T>
struct _conditionally_enabled_hash<T,
                                   false> {  // conditionally disabled hash
                                             // base
  constexpr _conditionally_enabled_hash() = delete;
  constexpr _conditionally_enabled_hash(const _conditionally_enabled_hash&) =
      delete;
  constexpr _conditionally_enabled_hash& operator=(
      const _conditionally_enabled_hash&) =
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
