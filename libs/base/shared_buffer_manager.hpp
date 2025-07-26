#pragma once

#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

class SharedBufferManager
{
public:
  typedef std::vector<uint8_t> shared_buffer_t;
  typedef std::shared_ptr<shared_buffer_t> shared_buffer_ptr_t;
  typedef std::list<shared_buffer_ptr_t> shared_buffer_ptr_list_t;
  typedef std::map<size_t, shared_buffer_ptr_list_t> shared_buffers_t;

private:
  std::mutex m_mutex;
  shared_buffers_t m_sharedBuffers;

public:
  static SharedBufferManager & instance();

  void clearReserved();

  shared_buffer_ptr_t reserveSharedBuffer(size_t s);
  void freeSharedBuffer(size_t s, shared_buffer_ptr_t buf);

  static uint8_t * GetRawPointer(shared_buffer_ptr_t ptr);
};
