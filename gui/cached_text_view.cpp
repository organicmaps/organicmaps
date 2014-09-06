#include "cached_text_view.hpp"
#include "controller.hpp"

#include "../geometry/transformations.hpp"

#include "../graphics/glyph.hpp"
#include "../graphics/screen.hpp"
#include "../graphics/display_list.hpp"
#include "../graphics/glyph_layout.hpp"


using namespace graphics;

namespace gui
{
  CachedTextView::CachedTextView(Params const & p)
    : Element(p)
  {
    m_text = p.m_text;
    m_uniText = strings::MakeUniString(p.m_text);

    setFont(EActive, FontDesc(12, Color(0, 0, 0, 255)));
    setFont(EPressed, FontDesc(12, Color(0, 0, 0, 255)));

    setColor(EActive, Color(Color(192, 192, 192, 255)));
    setColor(EPressed, Color(Color(64, 64, 64, 255)));
  }

  void CachedTextView::setText(string const & text)
  {
    if (m_text != text)
    {
      m_text = text;
      m_uniText = strings::MakeUniString(text);
      setIsDirtyLayout(true);
    }
  }

  void CachedTextView::setFont(EState state, FontDesc const & desc)
  {
    setIsDirtyLayout(true);
    Element::setFont(state, desc);
  }

  string const & CachedTextView::text() const
  {
    return m_text;
  }

  vector<m2::AnyRectD> const & CachedTextView::boundRects() const
  {
    if (isDirtyRect())
    {
      const_cast<CachedTextView*>(this)->layout();

      m_boundRects.clear();

      copy(m_layout->boundRects().begin(),
           m_layout->boundRects().end(),
           back_inserter(m_boundRects));
    }
    return m_boundRects;
  }

  void CachedTextView::draw(OverlayRenderer *r, math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      math::Matrix<double, 3, 3> id = math::Identity<double, 3>();

      if (m_maskedLayout)
        for (size_t i = 0; i < m_uniText.size(); ++i)
          r->drawDisplayList(m_maskedDls[i].get(),
                             math::Shift(id, m_maskedLayout->entries()[i].m_pt + m_maskedLayout->pivot()));

      for (size_t i = 0; i < m_uniText.size(); ++i)
        r->drawDisplayList(m_dls[i].get(),
                           math::Shift(id, m_layout->entries()[i].m_pt + m_layout->pivot()));
    }
  }

  void CachedTextView::cache()
  {
    layout();

    DisplayListCache * dlc = m_controller->GetDisplayListCache();
    FontDesc fontDesc = font(EActive);

    if (fontDesc.m_isMasked)
    {
      m_maskedDls.resize(m_uniText.size());
      for (size_t i = 0; i < m_uniText.size(); ++i)
        m_maskedDls[i] = dlc->FindGlyph(GlyphKey(m_uniText[i],
                                                 fontDesc.m_size,
                                                 fontDesc.m_isMasked,
                                                 fontDesc.m_isMasked ? fontDesc.m_maskColor : fontDesc.m_color));

      fontDesc.m_isMasked = false;
    }

    m_dls.resize(m_uniText.size());

    for (size_t i = 0; i < m_uniText.size(); ++i)
      m_dls[i] = dlc->FindGlyph(GlyphKey(m_uniText[i],
                                         fontDesc.m_size,
                                         fontDesc.m_isMasked,
                                         fontDesc.m_color));
  }

  void CachedTextView::purge()
  {
    m_dls.clear();
  }

  void CachedTextView::layout()
  {
    if (m_uniText.empty())
      return;

    FontDesc fontDesc = font(EActive);

    if (fontDesc.m_isMasked)
    {
      m_maskedLayout.reset(new GlyphLayout(m_controller->GetGlyphCache(),
                                           fontDesc,
                                           pivot(),
                                           m_uniText,
                                           position()));
      fontDesc.m_isMasked = false;
    }

    m_layout.reset(new GlyphLayout(m_controller->GetGlyphCache(),
                                   fontDesc,
                                   pivot(),
                                   m_uniText,
                                   position()));
  }

  void CachedTextView::setPivot(m2::PointD const & pv)
  {
    Element::setPivot(pv);
    if (m_maskedLayout)
      m_maskedLayout->setPivot(pivot());
    if (m_layout)
      m_layout->setPivot(pivot());
  }
}
