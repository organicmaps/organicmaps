#include "base/shared_buffer_manager.hpp"

SharedBufferManager & SharedBufferManager::instance()
{
  static SharedBufferManager i;
  return i;
}

void SharedBufferManager::clearReserved()
{
  std::lock_guard g(m_mutex);
  m_sharedBuffers.clear();
}

SharedBufferManager::shared_buffer_ptr_t SharedBufferManager::reserveSharedBuffer(size_t s)
{
  std::lock_guard g(m_mutex);

  shared_buffer_ptr_list_t & l = m_sharedBuffers[s];

  if (l.empty())
    l.push_back(std::make_shared<shared_buffer_t>(s));

  shared_buffer_ptr_t res = l.front();
  l.pop_front();

  return res;
}

void SharedBufferManager::freeSharedBuffer(size_t s, shared_buffer_ptr_t buf)
{
  std::lock_guard g(m_mutex);

  shared_buffer_ptr_list_t & l = m_sharedBuffers[s];

  l.push_back(buf);
}

uint8_t * SharedBufferManager::GetRawPointer(shared_buffer_ptr_t ptr)
{
  return &((*ptr)[0]);
}
