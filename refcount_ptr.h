#pragma once
#include  <cstddef>
#include "refcount_ptr_internals.h"

template<typename T>
class refcount_ptr {
public:
  constexpr refcount_ptr() noexcept : m_p_target(nullptr) {}
  constexpr refcount_ptr(nullptr_t) : refcount_ptr() {}
  refcount_ptr(T* p) : m_p_target(p) { add_ref(p); }
  template <class U> explicit refcount_ptr(U* p) : refcount_ptr(static_cast<T*>(p)) {}
  refcount_ptr (const refcount_ptr& x) noexcept : refcount_ptr(x.m_p_target) {}
  template <class U> refcount_ptr (const refcount_ptr<U>& x) noexcept : refcount_ptr(x.m_p_target) {}
  refcount_ptr (refcount_ptr&& x) noexcept : m_p_target(x.m_p_target) { x.m_p_target = nullptr; }
  template <class U> refcount_ptr (refcount_ptr<U>&& x) noexcept : refcount_ptr(static_cast<T*>(x.m_p_target)) {}

  ~refcount_ptr() {
    if (delete_ref_is_last(m_p_target))
      delete m_p_target;
  }

  refcount_ptr& operator=(const refcount_ptr& x) noexcept { clone_from(x); return *this; }
  template <class U> refcount_ptr& operator= (const refcount_ptr<U>& x) noexcept { clone_from(x); }

  refcount_ptr& operator= (refcount_ptr&& x) noexcept { move_from(x); return *this; }
  template <class U> refcount_ptr& operator= (refcount_ptr<U>&& x) noexcept { move_from(x); }

  T& operator*() const noexcept { return *m_p_target; }
  T* operator->() const noexcept { return m_p_target; }

  bool operator==(nullptr_t) const noexcept { return nullptr == m_p_target; }
  template <class U> bool operator== (const refcount_ptr<U>& rhs) const noexcept { return isEqualTo(rhs); }

  template <class U> bool operator!= (const refcount_ptr<U>& rhs) const noexcept { return !isEqualTo(rhs); }

  template <class... Args> static refcount_ptr<T> create(Args&&... args) {
    auto p = new T(args...);
    return refcount_ptr<T>(p);
  }

private:
  template <class U> void clone_from(const refcount_ptr<U>& x) { m_p_target = x.m_p_target; add_ref(m_p_target); }
  template <class U> void move_from(refcount_ptr<U>& x) { m_p_target = x.m_p_target; x.m_p_target = nullptr; }
  template <class U> bool isEqualTo(const refcount_ptr<U>& rhs) const noexcept { return m_p_target == rhs.m_p_target; }

  T* m_p_target;

template<typename U> friend class refcount_ptr;
};

