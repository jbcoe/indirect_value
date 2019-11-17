#ifndef ISOCPP_P1950_INDIRECT_VALUE_H
#define ISOCPP_P1950_INDIRECT_VALUE_H

#include <memory>
#include <utility>
#include <type_traits>

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

template <class T>
struct default_assign {
  T& operator()(T& lhs, const T& rhs) const { lhs = rhs; return lhs; }
};

template <class T, class C = default_copy<T>, class A= default_assign<T>, class D = std::default_delete<T>>
class indirect_value : private C, private A {
  
  std::unique_ptr<T, D> ptr_;

 public:
  indirect_value() = default;

  template <class... Ts>
  indirect_value(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  indirect_value(T* t, C c = C{}, A a = A{}, D d = D{}) : C(std::move(c)), A(std::move(a)), ptr_(std::unique_ptr<T, D>(t, std::move(d))) {
  }

  indirect_value(const indirect_value& i) : C(static_cast<const C&>(i)), A(static_cast<const A&>(i))  {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(static_cast<const C&>(i)(*i.ptr_), D{});
    }
  }

  indirect_value(indirect_value&& i) noexcept : C(std::move(static_cast<C&>(i))), A(std::move(static_cast<A&>(i))), ptr_(std::exchange(i.ptr_, nullptr)) {}

  indirect_value& operator = (const indirect_value& i) {
    if (!i) { 
      ptr_.reset();
    } else {
      // TODO: Can this be done more nicely?
      static_cast<C&>(*this) = static_cast<const C&>(i); 
      static_cast<A&>(*this) = static_cast<const A&>(i); 
      
      if (!ptr_){
        ptr_ = std::unique_ptr<T, D>(static_cast<const C&>(i)(*i.ptr_), D{});
      } else {
        static_cast<const A&>(i)(*ptr_, *i.ptr_);
      }
    }
    return *this;
  }

  indirect_value& operator = (indirect_value&& i) noexcept {
    static_cast<C&>(*this) = static_cast<C&&>(i); 
    static_cast<A&>(*this) = static_cast<A&&>(i); 
    ptr_ = std::exchange(i.ptr_, nullptr);
    return *this;
  }

  ~indirect_value() = default;

  T* operator->() { return ptr_.operator->(); }

  const T* operator->() const { return ptr_.operator->(); }

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  friend void swap(indirect_value& lhs, indirect_value& rhs) {
    using std::swap;
    swap(lhs.ptr_, rhs.ptr_);
    swap(lhs.c_, rhs.c_);
  }
};

}  // namespace isocpp_p1950

#endif  // ISOCPP_P1950_INDIRECT_VALUE_H
