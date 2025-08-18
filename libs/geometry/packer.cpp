#include "geometry/packer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

namespace m2
{
Packer::Packer()
  : m_currentX(0)
  , m_currentY(0)
  , m_yStep(0)
  , m_width(0)
  , m_height(0)
  , m_currentHandle(0)
  , m_maxHandle(0)
  , m_invalidHandle(0x00FFFFFF)
{}

Packer::Packer(unsigned width, unsigned height, uint32_t maxHandle)
  : m_currentX(0)
  , m_currentY(0)
  , m_yStep(0)
  , m_width(width)
  , m_height(height)
  , m_currentHandle(0)
  , m_maxHandle(maxHandle)
  , m_invalidHandle(0x00FFFFFF)
{}

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
  m_yStep = std::max(height, m_yStep);
  handle_t curHandle = m_currentHandle++;
  m_rects[curHandle] = m2::RectU(m_currentX, m_currentY, m_currentX + width, m_currentY + height);
  m_currentX += width;

  return curHandle;
}

Packer::handle_t Packer::freeHandle()
{
  if (m_currentHandle == m_maxHandle)
  {
    callOverflowFns();
    reset();
    m_currentHandle = 0;
  }

  return m_currentHandle++;
}

bool Packer::hasRoom(unsigned width, unsigned height) const
{
  return ((m_width >= width) && (m_height - m_currentY - m_yStep >= height)) ||
         ((m_width - m_currentX >= width) && (m_height - m_currentY >= height));
}

bool Packer::hasRoom(m2::PointU const * sizes, size_t cnt) const
{
  unsigned currentX = m_currentX;
  unsigned currentY = m_currentY;
  unsigned yStep = m_yStep;

  for (unsigned i = 0; i < cnt; ++i)
  {
    unsigned width = sizes[i].x;
    unsigned height = sizes[i].y;

    if (width <= m_width - currentX)
    {
      if (height <= m_height - currentY)
      {
        yStep = std::max(height, yStep);
        currentX += width;
      }
      else
        return false;
    }
    else
    {
      currentX = 0;
      currentY += yStep;
      yStep = 0;

      if (width <= m_width - currentX)
      {
        if (height <= m_height - currentY)
        {
          yStep = std::max(height, yStep);
          currentX += width;
        }
        else
          return false;
      }
    }
  }

  return true;
}

bool Packer::isPacked(handle_t handle)
{
  return m_rects.find(handle) != m_rects.end();
}

Packer::find_result_t Packer::find(handle_t handle) const
{
  rects_t::const_iterator it = m_rects.find(handle);
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
  LOG(LDEBUG, ("Texture|Handles Overflow"));
  overflowFns handlersCopy = m_overflowFns;
  while (!handlersCopy.empty())
  {
    handlersCopy.top().second();
    handlersCopy.pop();
  }
}
}  // namespace m2
