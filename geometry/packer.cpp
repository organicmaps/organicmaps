#include "packer.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"

namespace m2
{
  Packer::Packer()
    : m_currentX(0),
    m_currentY(0),
    m_width(0),
    m_height(0),
    m_currentHandle(0),
    m_maxHandle(0),
    m_invalidHandle(-1)
  {};

  Packer::Packer(unsigned width, unsigned height, uint32_t maxHandle)
    : m_currentX(0),
    m_currentY(0),
    m_yStep(0),
    m_width(width),
    m_height(height),
    m_currentHandle(0),
    m_maxHandle(maxHandle),
    m_invalidHandle(maxHandle + 1)
  {
  }

  uint32_t Packer::invalidHandle() const
  {
    return m_invalidHandle;
  }

  Packer::handle_t Packer::pack(unsigned width, unsigned height)
  {
    ASSERT(width <= m_width, ());
    ASSERT(height <= m_height, ());

    if ((width > m_width) || (height > m_height))
      return m_invalidHandle;

    /// checking whether there is enough space on X axis

    if (m_currentX + width > m_width)
    {
      m_currentY += m_yStep;
      m_currentX = 0;
      m_yStep = 0;
    }

    /// checking whether there is enough space on Y axis
    if (m_currentY + height > m_height)
    {
      /// texture overflow detected. all packed rects should be forgotten.
      callOverflowFns();
      reset();
    }
    /// check handleOverflow
    if (m_currentHandle == m_maxHandle)
    {
      /// handles overflow detected. all packed rects should be forgotten.
      callOverflowFns();
      reset();
      m_currentHandle = 0;
    }

    /// can pack
    m_yStep = max(height, m_yStep);
    handle_t curHandle = m_currentHandle++;
    m_rects[curHandle] = m2::RectU(m_currentX, m_currentY, m_currentX + width, m_currentY + height);
    m_currentX += width;

    return curHandle;
  }

  bool Packer::hasRoom(unsigned width, unsigned height) const
  {
    return ((m_width >= width) && (m_height - m_currentY - m_yStep >= height))
        || ((m_width - m_currentX >= width ) && (m_height - m_currentY >= height));
  }

  bool Packer::isPacked(handle_t handle)
  {
    return m_rects.find(handle) != m_rects.end();
  }

  Packer::find_result_t Packer::find(handle_t handle) const
  {
    unordered_map<handle_t, m2::RectU>::const_iterator it = m_rects.find(handle);
    std::pair<bool, m2::RectU> res;
    res.first = (it != m_rects.end());
    if (res.first)
      res.second = it->second;
    return res;
  }

  void Packer::remove(handle_t handle)
  {
    m_rects.erase(handle);
  }

  void Packer::reset()
  {
    m_rects.clear();
    m_currentX = 0;
    m_currentY = 0;
    m_yStep = 0;
    m_currentHandle = 0;
  }

  void Packer::addOverflowFn(overflowFn fn, int priority)
  {
    m_overflowFns.push(std::pair<size_t, overflowFn>(priority, fn));
  }

  void Packer::callOverflowFns()
  {
    LOG(LINFO, ("Texture|Handles Overflow"));
    overflowFns handlersCopy = m_overflowFns;
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second();
      handlersCopy.pop();
    }
  }
}
