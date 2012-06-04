#include "text_view.hpp"
#include "controller.hpp"

namespace gui
{
  TextView::TextView(Params const & p)
    : Element(p)
  {
    setText(p.m_text);

    setFont(EActive, yg::FontDesc(12, yg::Color(0, 0, 0, 255)));
    setFont(EPressed, yg::FontDesc(12, yg::Color(0, 0, 0, 255)));

    setColor(EActive, yg::Color(yg::Color(192, 192, 192, 255)));
    setColor(EPressed, yg::Color(yg::Color(64, 64, 64, 255)));
  }

  void TextView::setText(string const & text)
  {
    if (m_text != text)
    {
      m_text = text;
      setIsDirtyDrawing(true);
      setIsDirtyRect(true);
    }
  }

  string const & TextView::text() const
  {
    return m_text;
  }

  void TextView::cache()
  {
    yg::StraightTextElement::Params params;
    params.m_depth = depth();
    params.m_fontDesc = font(state());
    params.m_fontDesc.m_size *= visualScale();
    params.m_log2vis = true;
    params.m_pivot = pivot();
    params.m_position = position();
    params.m_glyphCache = m_controller->GetGlyphCache();
    params.m_logText = strings::MakeUniString(m_text);
    params.m_doSplit = true;
    params.m_delimiters = "\n";
    params.m_useAllParts = true;

    m_elem.reset(new yg::StraightTextElement(params));
  }

  void TextView::draw(yg::gl::OverlayRenderer *r, math::Matrix<double, 3, 3> const & m) const
  {
    checkDirtyDrawing();
    m_elem->draw(r, m);
  }

  vector<m2::AnyRectD> const & TextView::boundRects() const
  {
    if (isDirtyRect())
    {
      setIsDirtyDrawing(true);
      checkDirtyDrawing();
      m_boundRects.clear();
      m_boundRects.push_back(m2::AnyRectD(m_elem->roughBoundRect()));
      setIsDirtyRect(false);
    }

    return m_boundRects;
  }

  bool TextView::onTapStarted(m2::PointD const & pt)
  {
    return false;
  }

  bool TextView::onTapMoved(m2::PointD const & pt)
  {
    return false;
  }

  bool TextView::onTapEnded(m2::PointD const & pt)
  {
    return false;
  }

  bool TextView::onTapCancelled(m2::PointD const & pt)
  {
    return false;
  }
}
