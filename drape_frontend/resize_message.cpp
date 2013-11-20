#include "resize_message.hpp"

namespace df
{
  ResizeMessage::ResizeMessage(int x, int y, int w, int h)
    : m_rect(x, y, x + w, y + h)
  {
    SetType(Resize);
  }

  ResizeMessage::ResizeMessage(m2::RectI const & rect)
    : m_rect(rect)
  {
    SetType(Resize);
  }

  const m2::RectI & ResizeMessage::GetRect() const
  {
    return m_rect;
  }
}
