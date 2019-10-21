#ifndef JBCOE_CPP_INDIRECT
#define JBCOE_CPP_INDIRECT

#include <memory>
#include <utility>
#include <type_traits>

namespace jbcoe {

template <class T>
struct default_copy {
  T* operator()(const T& t) const { return new T(t); }
};

template <class T, class C = default_copy<T>, bool CanBeEmptyBaseClass = std::is_empty_v<C> && !std::is_final_v<C> >
class indirect_base {
protected:
  indirect_base() noexcept(noexcept(C())) = default;
  indirect_base(C c) : c_(std::move(c)) {}
  const C& get() const noexcept { return c_; }
  C c_;
};

template <class T, class C>
class indirect_base<T, C, true> : private C {
protected:
  indirect_base() noexcept(noexcept(C())) = default;
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

  indirect(T* t, C c = C{}, D d = D{}) : base(std::move(c)) {
    ptr_ = std::unique_ptr<T, D>(t, std::move(d));
  }

  indirect(const indirect& i) : base(get_c()) {
    if (i.ptr_) { 
      ptr_ = std::unique_ptr<T, D>(get_c()(*i.ptr_), D{});
    }
  }

  indirect(indirect&& i) : base(std::move(i)) {
    ptr_ = std::move(i.ptr_);
  }

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

  indirect& operator=(indirect&& i) {
    base::operator=(std::move(i));
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

  private:
    const C& get_c() const noexcept { return base::get(); }
};

}  // namespace jbcoe

#endif  // JBCOE_CPP_INDIRECT
