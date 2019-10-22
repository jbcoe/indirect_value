#ifndef JBCOE_CPP_INDIRECT
#define JBCOE_CPP_INDIRECT

#include <memory>
#include <utility>

namespace jbcoe {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

template <class T, class C = default_copy<T>, class D = std::default_delete<T>>
class indirect {
  std::unique_ptr<T, D> ptr_;
  C c_;

 public:
  indirect() = default;

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts) {
    ptr_ = std::unique_ptr<T, D>(new T(std::forward<Ts>(ts)...), D{});
  }

  indirect(T* t, C c = C{}, D d = D{}) : ptr_(std::unique_ptr<T, D>(t, std::move(d))), c_(std::move(c)) {
  }

  indirect(const indirect& i) : c_(i.c_) {
    if (i.ptr_) {
      ptr_ = std::unique_ptr<T, D>(i.c_(*i.ptr_), D{});
    }
  }

  indirect(indirect&& i) noexcept : ptr_(std::exchange(i.ptr_, nullptr)), c_(std::exchange(i.c_, C{})) {
  }

  indirect& operator = (const indirect& i) {
    c_ = i.c_;
    if (i.ptr_) { 
      if (!ptr_){
        ptr_ = std::unique_ptr<T, D>(i.c_(*i.ptr_), D{});
      }
      else{
        *ptr_ = *i.ptr_;
      }
    }
    return *this;
  }

  indirect& operator = (indirect&& i) noexcept {
    c_ = std::exchange(i.c_, C{});
    ptr_ = std::exchange(i.ptr_, nullptr);
    return *this;
  }

  ~indirect() = default;

  T* operator->() { return ptr_.operator->(); }

  const T* operator->() const { return ptr_.operator->(); }

  T& operator*() { return *ptr_; }

  const T& operator*() const { return *ptr_; }

  friend void swap(indirect& lhs, indirect& rhs) {
    using std::swap;
    swap(lhs.ptr_, rhs.ptr_);
    swap(lhs.c_, rhs.c_);
  }
};

}  // namespace jbcoe

#endif  // JBCOE_CPP_INDIRECT
