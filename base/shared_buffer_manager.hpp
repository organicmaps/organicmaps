#pragma once

#include "base/mutex.hpp"
#include "std/vector.hpp"
#include "std/shared_ptr.hpp"
#include "std/list.hpp"
#include "std/map.hpp"

class SharedBufferManager
{
public:
  typedef vector<uint8_t> shared_buffer_t;
  typedef shared_ptr<shared_buffer_t> shared_buffer_ptr_t;
  typedef list<shared_buffer_ptr_t> shared_buffer_ptr_list_t;
  typedef map<size_t, shared_buffer_ptr_list_t> shared_buffers_t;
private:

  threads::Mutex m_mutex;
  shared_buffers_t m_sharedBuffers;

public:
  static SharedBufferManager & instance();

  void clearReserved();

  shared_buffer_ptr_t reserveSharedBuffer(size_t s);
  void freeSharedBuffer(size_t s, shared_buffer_ptr_t buf);

  static uint8_t * GetRawPointer(shared_buffer_ptr_t ptr);
};
