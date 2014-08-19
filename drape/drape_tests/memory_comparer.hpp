#pragma once

#include "../../testing/testing.hpp"
#include "../glconstants.hpp"
#include "../../base/logging.hpp"

#include "../../std/cstring.hpp"

namespace dp
{
  struct MemoryComparer
  {
    void * m_mem;
    int m_size;

    MemoryComparer(void * memory, int size)
      : m_mem(memory)
      , m_size(size)
    {
    }

    void cmpSubBuffer(glConst /*type*/, uint32_t size, void const * data, uint32_t /*offset*/) const
    {
      TEST_EQUAL(size, m_size, ());
      TEST_EQUAL(memcmp(m_mem, data, size), 0, ());
    }

    void cmpSubImage(uint32_t /*x*/, uint32_t /*y*/, uint32_t /*width*/, uint32_t /*height*/,
                     glConst /*layout*/, glConst /*pixelFormat*/, void const * data) const
    {
      TEST_EQUAL(memcmp(m_mem, data, m_size), 0, ());
    }
  };

}
