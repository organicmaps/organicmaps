#include "base/thread_checker.hpp"

ThreadChecker::ThreadChecker() : m_id(this_thread::get_id()) {}

bool ThreadChecker::CalledOnOriginalThread() const { return this_thread::get_id() == m_id; }
