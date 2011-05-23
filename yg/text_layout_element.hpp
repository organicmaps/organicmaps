#pragma once

#include "layout_element.hpp"
#include "skin.hpp"
#include "font_desc.hpp"

#include "../std/shared_ptr.hpp"

namespace yg
{
  class Skin;
  class ResourceManager;

  class TextLayoutElement : public LayoutElement
  {
  private:

    wstring m_text;
    double m_depth;
    FontDesc m_fontDesc;
    shared_ptr<Skin> m_skin;
    shared_ptr<ResourceManager> m_rm;
    bool m_log2vis;
    m2::RectD m_limitRect;

  public:

    TextLayoutElement(
      char const * text,
      double depth,
      FontDesc const & fontDesc,
      bool log2vis,
      shared_ptr<Skin> const & skin,
      shared_ptr<ResourceManager> const & rm,
      m2::PointD const & pivot,
      yg::EPosition pos);

    m2::RectD const boundRect() const;

    void draw(Screen * screen);
  };
}
