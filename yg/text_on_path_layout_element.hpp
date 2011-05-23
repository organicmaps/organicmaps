#pragma once

#include "text_layout_element.hpp"
#include "../std/shared_ptr.hpp"

namespace yg
{
  struct FontDesc;
  class TextPath;

  class TextOnPathLayoutElement : public TextLayoutElement
  {
  public:
    TextOnPathLayoutElement(shared_ptr<TextPath> const & path,
                            char const * text,
                            double depth,
                            FontDesc const & fontDesc);

    m2::RectD const boundRect() const;

    void draw(Screen * screen);
  };
}
