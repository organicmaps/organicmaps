#pragma once

#include "base/logging.hpp"
#include "drape/gl_constants.hpp"
#include "testing/testing.hpp"

#include <cstring>

namespace dp
{
struct MemoryComparer
{
  void * m_mem;
  int m_size;

  MemoryComparer(void * memory, int size) : m_mem(memory), m_size(size) {}
  void cmpSubBuffer(glConst /*type*/, uint32_t size, void const * data, uint32_t /*offset*/) const
  {
    TEST_EQUAL(size, static_cast<uint32_t>(m_size), ());
    TEST_EQUAL(memcmp(m_mem, data, size), 0, ());
  }

  void cmpSubImage(uint32_t /*x*/, uint32_t /*y*/, uint32_t width, uint32_t height, glConst layout, glConst pixelFormat,
                   void const * data) const
  {
    uint32_t channelCount = 0;
    if (layout == gl_const::GLRGBA || layout == gl_const::GLRGBA8 || layout == gl_const::GLRGBA4)
      channelCount = 4;
    else if (layout == gl_const::GLRGB)
      channelCount = 3;
    else if (layout == gl_const::GLAlpha || layout == gl_const::GLAlpha8 || layout == gl_const::GLLuminance ||
             layout == gl_const::GLLuminance8 || layout == gl_const::GLAlphaLuminance || layout == gl_const::GLRed)
    {
      channelCount = 1;
    }
    else
      ASSERT(false, ());

    ASSERT(gl_const::GL8BitOnChannel == pixelFormat, ());

    TEST_EQUAL(static_cast<uint32_t>(m_size), width * height * channelCount, ());
    uint8_t * member = (uint8_t *)m_mem;
    uint8_t * input = (uint8_t *)data;

    for (int i = 0; i < m_size; ++i)
      TEST_EQUAL(member[i], input[i], (i));
  }
};
}  // namespace dp
