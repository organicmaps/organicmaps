#include "base/shared_buffer_manager.hpp"

#include <bit>

SharedBufferManager & SharedBufferManager::instance()
{
  static SharedBufferManager i;
  return i;
}

void SharedBufferManager::clearReserved()
{
  std::lock_guard g(m_mutex);
  m_pool.clear();
}

SharedBufferManager::shared_buffer_ptr_t SharedBufferManager::reserveSharedBuffer(size_t s)
{
  auto const normalized = std::bit_ceil(s);
  std::lock_guard g(m_mutex);

  auto & bucket = m_pool[normalized];

  if (bucket.empty())
    return std::make_shared<shared_buffer_t>(normalized);

  auto result = std::move(bucket.back());
  bucket.pop_back();
  return result;
}

void SharedBufferManager::freeSharedBuffer(size_t s, shared_buffer_ptr_t buf)
{
  auto const normalized = std::bit_ceil(s);
  std::lock_guard g(m_mutex);
  m_pool[normalized].push_back(std::move(buf));
}

uint8_t * SharedBufferManager::GetRawPointer(shared_buffer_ptr_t const & ptr)
{
  return ptr->data();
}
