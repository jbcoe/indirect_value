#ifndef JBCOE_CPP_INDIRECT
#define JBCOE_CPP_INDIRECT

#include <memory>

namespace jbcoe {

template <class T>
struct default_copy {
  T* operator()(const T& t) { return new T(t); }
};

template <class T, class C = default_copy<T>, class D = std::default_delete<T>>
class indirect {
  std::unique_ptr<T, D> ptr_;
  C c_;

 public:
  indirect() = default;

  template <class T>
  indirect(std::in_place<T>, Ts...&& ts) : c_(std::move(c)) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...) D{});
  }

  indirect(T* t, C c = C{}, D d = D{}) : c_(std::move(c)) {
    ptr_ = std::unique_ptr<T, D>(t, std::move(d));
  }

  indirect(const indirect& i) : c_(C{}) {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(i.c_(*i.ptr_), D{});
    }
  }

  indirect(indirect&& i) : c_(std::move(i.c_)) {
    ptr_ = std::move(i.ptr_);
  }

  indirect& operator = (const indirect& i) {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(i.c_(*i.ptr_), D{});
    }
  }

  indirect& operator = (indirect&& i) {
    c_ = C(std::move(i.c_));
    ptr_ = std::move(i.ptr_);
  }

  ~indirect() = default;

  T* operator->() { return ptr_.operator->(); }

  const T* operator->() const { return ptr_.operator->(); }

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }
};

}  // namespace jbcoe

#endif  // JBCOE_CPP_INDIRECT