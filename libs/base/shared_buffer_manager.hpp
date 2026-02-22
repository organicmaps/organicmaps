#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class SharedBufferManager
{
public:
  using shared_buffer_t = std::vector<uint8_t>;
  using shared_buffer_ptr_t = std::unique_ptr<shared_buffer_t>;

private:
  std::mutex m_mutex;
  // Use std::flat_map when Apple Clang supports it.
  std::unordered_map<size_t, std::vector<shared_buffer_ptr_t>> m_pool;

public:
  static SharedBufferManager & Instance();

  void ClearReserved();

  shared_buffer_ptr_t ReserveSharedBuffer(size_t s);
  void FreeSharedBuffer(size_t s, shared_buffer_ptr_t buf);

  static uint8_t * GetRawPointer(shared_buffer_ptr_t const & ptr) { return ptr->data(); }
};
