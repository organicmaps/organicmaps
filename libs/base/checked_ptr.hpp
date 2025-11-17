#pragma once

#include "base/assert.hpp"

#include <memory>
#include <thread>

// Make separate non-inline functions intentionally, to catch crash reasons from Android stack traces only.
// Expect to see one of these functions in a crash dump.
namespace check_impl
{
void not_null(void const * p);
void equal_thread(std::thread::id id);
}  // namespace check_impl

template <class T>
class CheckedPtr
{
  std::unique_ptr<T> m_ptr;
  std::thread::id m_id;

public:
  void Assign(T * p)
  {
    CHECK(!m_ptr, ());
    m_id = std::this_thread::get_id();
    m_ptr.reset(p);
  }

  operator bool() const { return m_ptr != nullptr; }

  T * operator->() const
  {
    check_impl::not_null(m_ptr.get());
    return m_ptr.get();
  }

  T * thread_check() const
  {
    check_impl::not_null(m_ptr.get());
    check_impl::equal_thread(m_id);
    return m_ptr.get();
  }
};
