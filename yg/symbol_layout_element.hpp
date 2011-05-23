#pragma once

#include "layout_element.hpp"

namespace yg
{
  class SymbolLayoutElement : public LayoutElement
  {
  public:
    SymbolLayoutElement();

    m2::RectD const boundRect() const;

    void draw(Screen * screen);
  };
}
