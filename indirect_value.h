#ifndef ISOCPP_P1950_INDIRECT_VALUE_H
#define ISOCPP_P1950_INDIRECT_VALUE_H

#include <memory>
#include <type_traits>
#include <utility>

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
  indirect_value() = default;

  template <class... Ts>
  explicit indirect_value(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  template <class U, class = std::enable_if_t<std::is_same_v<T, U>>>
  explicit indirect_value(U* u, C c = C{}, D d = D{}) noexcept
      : base(std::move(c)), ptr_(std::unique_ptr<T, D>(u, std::move(d))) {}

  explicit indirect_value(const indirect_value& i)
      ISOCPP_P1950_REQUIRES(std::is_copy_constructible_v<T>)
      : base(i.get_c()) {
    if (i.ptr_) {
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), i.ptr_.get_deleter());
    }
  }

  explicit indirect_value(indirect_value&& i) noexcept
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

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  void swap(indirect_value& rhs) noexcept(
      std::is_nothrow_swappable_v<C>&& std::is_nothrow_swappable_v<D>) {
    using std::swap;
    swap(get_c(), rhs.get_c());
    swap(ptr_, rhs.ptr_);
  }

  template<class TC = C>
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

}  // namespace isocpp_p1950

#endif  // ISOCPP_P1950_INDIRECT_VALUE_H
