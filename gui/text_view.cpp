#include "gui/text_view.hpp"
#include "gui/controller.hpp"

#include "graphics/display_list.hpp"
#include "graphics/screen.hpp"
#include "graphics/straight_text_element.hpp"

#include "geometry/transformations.hpp"


namespace gui
{
  TextView::TextView(Params const & p)
    : Element(p), m_maxWidth(numeric_limits<unsigned>::max())
  {
    setText(p.m_text);

    setFont(EActive, graphics::FontDesc(12, graphics::Color(0, 0, 0, 255)));
    setFont(EPressed, graphics::FontDesc(12, graphics::Color(0, 0, 0, 255)));

    setColor(EActive, graphics::Color(graphics::Color(192, 192, 192, 255)));
    setColor(EPressed, graphics::Color(graphics::Color(64, 64, 64, 255)));
  }

  void TextView::setText(string const & text)
  {
    if (m_text != text)
    {
      m_text = text;
      setIsDirtyLayout(true);
    }
  }

  string const & TextView::text() const
  {
    return m_text;
  }

  void TextView::layoutBody(EState state)
  {
    unique_ptr<graphics::StraightTextElement> & elem = m_elems[state];

    graphics::StraightTextElement::Params params;

    params.m_depth = depth();
    params.m_fontDesc = font(state);
    params.m_fontDesc.m_size *= visualScale();
    params.m_log2vis = true;
    params.m_pivot = m2::PointD(0.0, 0.0);
    params.m_position = position();
    params.m_glyphCache = m_controller->GetGlyphCache();
    params.m_logText = strings::MakeUniString(m_text);
    params.m_doSplit = true;
    params.m_doForceSplit = true;
    params.m_delimiters = "\n";
    params.m_useAllParts = true;
    params.m_maxPixelWidth = m_maxWidth;

    elem.reset(new graphics::StraightTextElement(params));
  }

  void TextView::layout()
  {
    layoutBody(EActive);
    layoutBody(EPressed);
  }

  void TextView::cacheBody(EState state)
  {
    graphics::Screen * cs = m_controller->GetCacheScreen();

    unique_ptr<graphics::DisplayList> & dl = m_dls[state];

    dl.reset();
    dl.reset(cs->createDisplayList());

    cs->beginFrame();
    cs->setDisplayList(dl.get());

    m_elems[state]->draw(cs, math::Identity<double, 3>());

    cs->setDisplayList(0);
    cs->endFrame();
  }

  void TextView::cache()
  {
    cacheBody(EActive);
    cacheBody(EPressed);
  }

  void TextView::purge()
  {
    m_dls.clear();
  }

  void TextView::draw(graphics::OverlayRenderer *r, math::Matrix<double, 3, 3> const & m) const
  {
    if (isVisible())
    {
      checkDirtyLayout();

      math::Matrix<double, 3, 3> drawM = math::Shift(math::Identity<double, 3>(),
                                                     pivot());

      r->drawDisplayList(m_dls.at(state()).get(), drawM * m);
    }
  }

  void TextView::GetMiniBoundRects(RectsT & rects) const
  {
    checkDirtyLayout();

    m2::PointD const pt = pivot();

    rects.push_back(m2::AnyRectD(Offset(m_elems.at(EActive)->GetBoundRect(), pt)));
    rects.push_back(m2::AnyRectD(Offset(m_elems.at(EPressed)->GetBoundRect(), pt)));
  }

  void TextView::setMaxWidth(unsigned width)
  {
    m_maxWidth = width;
    setIsDirtyLayout(true);
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
