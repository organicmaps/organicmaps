#pragma once

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

class SharedBufferManager
{
public:
  using shared_buffer_t = std::vector<uint8_t>;

  // Custom deleter that returns the buffer to the manager's pool.
  // The deleter accesses Instance() during pointer destruction, so a shared_buffer_ptr_t must not
  // outlive the SharedBufferManager singleton. Instance() is a function-local static; do not store
  // a shared_buffer_ptr_t in static-storage-duration objects unless their destruction is sequenced
  // before Instance()'s.
  struct Deleter
  {
    void operator()(shared_buffer_t * p) const noexcept;
  };
  using shared_buffer_ptr_t = std::unique_ptr<shared_buffer_t, Deleter>;

  static SharedBufferManager & Instance();

  void ClearReserved();

  // Rounds up the requested size to the nearest power of 2 (for better buffer reuse)
  // and returns a buffer of that size from the pool if available, or creates a new one if not.
  // The returned pointer self-returns to the pool on destruction via Deleter.
  shared_buffer_ptr_t ReserveSharedBuffer(size_t s);

  static uint8_t * GetRawPointer(shared_buffer_ptr_t const & ptr) { return ptr->data(); }

private:
  // Internal pool storage uses default-deleter unique_ptr. If pool entries used the public Deleter,
  // destroying m_pool (via ClearReserved or singleton teardown) would recurse back into the pool
  // that is being torn down.
  using pool_buffer_ptr_t = std::unique_ptr<shared_buffer_t>;

  // Returns a raw buffer to the pool. Called only by Deleter::operator(); not part of the public API.
  void ReturnToPool(shared_buffer_t * raw) noexcept;

  std::mutex m_mutex;
  // Use std::flat_map when Apple Clang supports it.
  std::unordered_map<size_t, std::vector<pool_buffer_ptr_t>> m_pool;
};
