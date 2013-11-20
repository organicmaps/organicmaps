#pragma once

#include "message.hpp"

#include "../geometry/rect2d.hpp"

namespace df
{
  class ResizeMessage : public Message
  {
  public:
    ResizeMessage(int x, int y, int w, int h);
    ResizeMessage(m2::RectI const & rect);

    const m2::RectI & GetRect() const;

  private:
    m2::RectI m_rect;
  };
}
