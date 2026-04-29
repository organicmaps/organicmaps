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
    return shared_buffer_ptr_t{new shared_buffer_t(normalized)};

  auto result = std::move(bucket.back());
  bucket.pop_back();
  return shared_buffer_ptr_t{result.release()};
}

void SharedBufferManager::ReturnToPool(shared_buffer_t * raw) noexcept
{
  // raw is owned solely by us at this point. Wrap it back into a default-deleter ptr for storage.
  pool_buffer_ptr_t buf{raw};
  std::lock_guard g(m_mutex);
  m_pool[buf->size()].push_back(std::move(buf));
}

void SharedBufferManager::Deleter::operator()(shared_buffer_t * p) const noexcept
{
  if (p != nullptr)
    Instance().ReturnToPool(p);
}
