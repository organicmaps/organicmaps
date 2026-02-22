#include "base/shared_buffer_manager.hpp"

#include <bit>

SharedBufferManager & SharedBufferManager::Instance()
{
  static SharedBufferManager i;
  return i;
}

void SharedBufferManager::ClearReserved()
{
  std::lock_guard g(m_mutex);
  m_pool.clear();
}

SharedBufferManager::shared_buffer_ptr_t SharedBufferManager::ReserveSharedBuffer(size_t s)
{
  auto const normalized = std::bit_ceil(s);
  std::lock_guard g(m_mutex);

  auto & bucket = m_pool[normalized];

  if (bucket.empty())
    return std::make_unique<shared_buffer_t>(normalized);

  auto result = std::move(bucket.back());
  bucket.pop_back();
  return result;
}

void SharedBufferManager::FreeSharedBuffer(size_t s, shared_buffer_ptr_t buf)
{
  auto const normalized = std::bit_ceil(s);
  std::lock_guard g(m_mutex);
  m_pool[normalized].push_back(std::move(buf));
}
