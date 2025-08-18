#include "base/thread_checker.hpp"

ThreadChecker::ThreadChecker() : m_id(std::this_thread::get_id()) {}

bool ThreadChecker::CalledOnOriginalThread() const
{
  return std::this_thread::get_id() == m_id;
}
