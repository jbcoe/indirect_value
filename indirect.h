#ifndef ISOCPP_P1950_INDIRECT_H
#define ISOCPP_P1950_INDIRECT_H

#include <memory>
#include <utility>
#include <type_traits>

namespace isocpp_p1950 {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

template <class T, class C = default_copy<T>, bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C> >
class indirect_base {
protected:
  template<class U = C, class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_base() noexcept(noexcept(C())) {}
  indirect_base(C c) : c_(std::move(c)) {}
  const C& get() const noexcept { return c_; }
  C c_;
};

template <class T, class C>
class indirect_base<T, C, true> : private C {
protected:
  template<class U=C, class = std::enable_if_t<std::is_default_constructible_v<U>>>
  indirect_base() noexcept(noexcept(C())) {}
  indirect_base(C c) : C(std::move(c)) {}
  const C& get() const noexcept { return *this; }
};

template <class T, class C = default_copy<T>, class D = std::default_delete<T>>
class indirect : private indirect_base<T, C> {
  
  using base = indirect_base<T, C>;
  
  std::unique_ptr<T, D> ptr_;

 public:
  indirect() = default;

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  indirect(T* t, C c = C{}, D d = D{}) : base(std::move(c)), ptr_(std::unique_ptr<T, D>(t, std::move(d))) {
  }

  indirect(const indirect& i) : base(get_c()) {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), D{});
    }
  }

  indirect(indirect&& i) noexcept : base(std::move(i)), ptr_(std::exchange(i.ptr_, nullptr)) {}

  indirect& operator = (const indirect& i) {
    base::operator=(i);
    if (i.ptr_) { 
      if (!ptr_){
        ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), D{});
      }
      else{
        *ptr_ = *i.ptr_;
      }
    }
    return *this;
  }

  indirect& operator = (indirect&& i) noexcept {
    base::operator=(std::move(i));
    ptr_ = std::exchange(i.ptr_, nullptr);
    return *this;
  }

  ~indirect() = default;

  T* operator->() { return ptr_.operator->(); }

  const T* operator->() const { return ptr_.operator->(); }

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }

  explicit constexpr operator bool() const noexcept { return ptr_ != nullptr; }

  friend void swap(indirect& lhs, indirect& rhs) {
    using std::swap;
    swap(lhs.ptr_, rhs.ptr_);
    swap(lhs.c_, rhs.c_);
  }

  private:
    const C& get_c() const noexcept { return base::get(); }
};

}  // namespace isocpp_p1950

#endif  // ISOCPP_P1950_INDIRECT_H
