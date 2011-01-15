#include "drawer_yg.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/scales.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/profiler.hpp"
#include "../base/logging.hpp"

#include "../std/sstream.hpp"

#include "../base/start_mem_debug.hpp"

DrawerYG::Params::Params()
  : m_dynamicPagesCount(2),
    m_textPagesCount(2)
{
}

DrawerYG::DrawerYG(string const & skinName, params_t const & params)
{
  m_pScreen = shared_ptr<yg::gl::Screen>(new yg::gl::Screen(params));
  m_pSkin = shared_ptr<yg::Skin>(loadSkin(params.m_resourceManager,
                                          skinName,
                                          params.m_dynamicPagesCount,
                                          params.m_textPagesCount));
  m_pScreen->setSkin(m_pSkin);

  if (m_pSkin)
    m_pSkin->addClearPageFn(&DrawerYG::ClearSkinPage, 0);
}

namespace
{
  struct make_invalid
  {
    uint32_t m_pageIDMask;

    make_invalid(uint8_t pageID) : m_pageIDMask(pageID << 24)
    {}

    void operator() (int, int, drule::BaseRule * p)
    {
      if ((p->GetID() & 0xFF000000) == m_pageIDMask)
        p->MakeEmptyID();
    }
  };
}

void DrawerYG::ClearSkinPage(uint8_t pageID)
{
  drule::rules().ForEachRule(make_invalid(pageID));
}

void DrawerYG::beginFrame()
{
  m_pScreen->beginFrame();
}

void DrawerYG::endFrame()
{
  m_pScreen->endFrame();
  m_pathsOrg.clear();
}

void DrawerYG::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
{
  m_pScreen->clear(c, clearRT, depth, clearDepth);
}

void DrawerYG::onSize(int w, int h)
{
  m_pScreen->onSize(w, h);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, string const & symbolName, int depth)
{
  m_pScreen->drawPoint(pt, m_pSkin->mapSymbol(symbolName.c_str()), depth);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, rule_ptr_t pRule, int depth)
{
  // Use BaseRule::m_id to cache for point draw rule.
  // This rules doesn't mix with other rule-types.

  uint32_t id = pRule->GetID();
  if (id == drule::BaseRule::empty_id)
  {
    string name;
    pRule->GetSymbol(name);
    id = m_pSkin->mapSymbol(name.c_str());

    if (id != drule::BaseRule::empty_id)
      pRule->SetID(id);
    else
    {
      //ASSERT ( false, ("Can't find symbol by id = ", (name)) );
      return;
    }
  }

  m_pScreen->drawPoint(pt, id, depth);
}

void DrawerYG::drawPath(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth)
{
  // Use BaseRule::m_id to cache for line draw rule.
  // This rule type is used for line and area drawing, so leave m_id for line type.

  uint32_t id = pRule->GetID();
  if (id == drule::BaseRule::empty_id)
  {
    vector<double> pattern;
    double offset;
    pRule->GetPattern(pattern, offset);

    for (size_t i = 0; i < pattern.size(); ++i)
      pattern[i] *= m_scale * m_visualScale;

    yg::PenInfo rd(yg::Color::fromXRGB(pRule->GetColor(), pRule->GetAlpha()),
                   max(pRule->GetWidth() * m_scale, 1.0) * m_visualScale,
                   pattern.empty() ? 0 : &pattern[0], pattern.size(), offset * m_scale);

    id = m_pSkin->mapPenInfo(rd);

    ASSERT ( id != drule::BaseRule::empty_id, () );
    pRule->SetID(id);
  }

  m_pScreen->drawPath(&pts[0], pts.size(), id, depth);
}

void DrawerYG::drawArea(vector<m2::PointD> const & pts, rule_ptr_t pRule, int depth)
{
  // DO NOT cache 'id' in pRule, because one rule can use in drawPath and drawArea.
  // Leave CBaseRule::m_id for drawPath. mapColor working fast enough.

  uint32_t const id = m_pSkin->mapColor(yg::Color::fromXRGB(pRule->GetFillColor(), pRule->GetAlpha()));
  ASSERT ( id != -1, () );

  m_pScreen->drawTrianglesList(&pts[0], pts.size()/*, res*/, id, depth);
}

namespace
{
  double const min_text_height = 7.99;      // 8
  double const min_text_height_mask = 9.99; // 10
}

uint8_t DrawerYG::get_text_font_size(rule_ptr_t pRule) const
{
  double const h = pRule->GetTextHeight() * m_scale;
  return my::rounds(max(h, min_text_height) * m_visualScale);
}

uint8_t DrawerYG::get_pathtext_font_size(rule_ptr_t pRule) const
{
  double const h = pRule->GetTextHeight() * m_scale - 2.0;
  return my::rounds(max(h, min_text_height) * m_visualScale);
}

void DrawerYG::drawText(m2::PointD const & pt, string const & name, rule_ptr_t pRule, int depth)
{
  m_pScreen->drawText(pt, 0.0, get_text_font_size(pRule), name, depth);
}

bool DrawerYG::drawPathText(di::PathInfo const & info, string const & name, uint8_t fontSize, int /*depth*/)
{
  bool const isMasked = (double(fontSize) / m_visualScale >= min_text_height_mask);

  return m_pScreen->drawPathText( &info.m_path[0], info.m_path.size(), fontSize, name,
                                  info.GetLength(), info.GetOffset(),
                                  yg::gl::Screen::middle_line, isMasked, yg::maxDepth);
}

shared_ptr<yg::gl::Screen> DrawerYG::screen() const
{
  return m_pScreen;
}

void DrawerYG::SetVisualScale(double visualScale)
{
  m_visualScale = visualScale;
}

void DrawerYG::SetScale(int level)
{
  m_scale = scales::GetM2PFactor(level);
}

void DrawerYG::Draw(di::DrawInfo const * pInfo, rule_ptr_t pRule, int depth)
{
  bool const isCaption = pRule->GetTextHeight() >= 0.0;

  string symbol;
  pRule->GetSymbol(symbol);
  bool const isSymbol = !symbol.empty();

  bool const isPath = !pInfo->m_pathes.empty();
  bool const isArea = !pInfo->m_areas.empty();
  bool const isName = !pInfo->m_name.empty();

  if (!isCaption)
  {
    // draw path
    if (isPath && !isSymbol && (pRule->GetColor() != -1))
    {
      for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
        drawPath(i->m_path, pRule, depth);
    }

    // draw area
    if (isArea)
    {
      bool const isFill = pRule->GetFillColor() != -1;
      bool isSym = isSymbol && ((pRule->GetType() & drule::way) != 0);

      for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
      {
        if (isFill)
          drawArea(i->m_path, pRule, depth);
        else if (isSym)
          drawSymbol(i->GetCenter(), pRule, depth);
      }
    }

    // draw point symbol
    if (!isPath && !isArea && isSymbol && ((pRule->GetType() & drule::node) != 0))
      drawSymbol(pInfo->m_point, pRule, depth);
  }
  else
  {
    if (isName)
    {
      bool isN = ((pRule->GetType() & drule::way) != 0);

      // draw area text
      if (isArea && isN)
      {
        for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
          drawText(i->GetCenter(), pInfo->m_name, pRule, depth);
      }

      // draw way name
      if (isPath && !isArea && isN)
      {
        for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
        {
          uint8_t const fontSize = get_pathtext_font_size(pRule);

          list<m2::RectD> & lst = m_pathsOrg[pInfo->m_name];

          m2::RectD r = i->GetLimitRect();
          r.Inflate(-r.SizeX() / 4.0, -r.SizeY() / 4.0);
          r.Inflate(fontSize, fontSize);

          bool needDraw = true;
          for (list<m2::RectD>::const_iterator j = lst.begin(); j != lst.end(); ++j)
            if (r.IsIntersect(*j))
            {
              needDraw = false;
              break;
            }

          if (needDraw && drawPathText(*i, pInfo->m_name, fontSize, depth))
            lst.push_back(r);
        }
      }

      // draw point text
      isN = ((pRule->GetType() & drule::node) != 0);
      if (!isPath && !isArea && isN)
        drawText(pInfo->m_point, pInfo->m_name, pRule, depth);
    }
  }
}

void DrawerYG::drawStats(double duration, int scale, double lat, double lng, m2::PointD const & pt, bool log2vis)
{
  ostringstream out;
  out << "SPF: " << duration;
  if (duration == 0.0)
    out << " FPS: inf";
  else
    out << " FPS: " << 1.0 / duration;
  m_pScreen->drawText(pt + m2::PointD(10, 20), 0, 10, out.str().c_str(), yg::maxDepth, true, log2vis);

  out.str("");
  out << "(" << lat << ", " << lng << ") Scale: " << scale;
  m_pScreen->drawText(pt + m2::PointD(10, 40), 0, 10, out.str().c_str(), yg::maxDepth, true, log2vis);
}
