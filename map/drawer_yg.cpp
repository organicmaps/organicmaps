#include "drawer_yg.hpp"

#include "../std/bind.hpp"

#include "../indexer/drawing_rules.hpp"
#include "../indexer/scales.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/circle_info.hpp"
#include "../yg/pen_info.hpp"

#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/buffer_vector.hpp"

DrawerYG::Params::Params()
  : m_dynamicPagesCount(2),
    m_textPagesCount(2),
    m_threadID(0),
    m_visualScale(1)
{
}

uint32_t di::DrawRule::GetID(size_t threadID) const
{
  return (m_transparent ? m_rule->GetID2(threadID) : m_rule->GetID(threadID));
}

void di::DrawRule::SetID(size_t threadID, uint32_t id) const
{
  m_transparent ? m_rule->SetID2(threadID, id) : m_rule->SetID(threadID, id);
}

DrawerYG::DrawerYG(params_t const & params)
  : m_visualScale(params.m_visualScale), m_threadID(params.m_threadID)
{
  m_pScreen = shared_ptr<yg::gl::Screen>(new yg::gl::Screen(params));
  m_pSkin = shared_ptr<yg::Skin>(loadSkin(params.m_resourceManager,
                                          params.m_skinName,
                                          params.m_dynamicPagesCount,
                                          params.m_textPagesCount));
  m_pScreen->setSkin(m_pSkin);

  if (m_pSkin)
    m_pSkin->addClearPageFn(bind(&DrawerYG::ClearSkinPage, m_threadID, _1), 0);
}

namespace
{
  struct DoMakeInvalidRule
  {
    size_t m_threadID;
    uint32_t m_pipelineIDMask;

    DoMakeInvalidRule(size_t threadID, uint8_t pipelineID)
      : m_threadID(threadID), m_pipelineIDMask(pipelineID << 24)
    {}

    void operator() (int, int, int, drule::BaseRule * p)
    {
      if ((p->GetID(m_threadID) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID(m_threadID);
      if ((p->GetID2(m_threadID) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID2(m_threadID);
    }
  };
}

void DrawerYG::ClearSkinPage(size_t threadID, uint8_t pipelineID)
{
  drule::rules().ForEachRule(DoMakeInvalidRule(threadID, pipelineID));
}

void DrawerYG::beginFrame()
{
  m_pScreen->beginFrame();
}

void DrawerYG::endFrame()
{
  m_pScreen->endFrame();
}

void DrawerYG::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
{
  m_pScreen->clear(c, clearRT, depth, clearDepth);
}

void DrawerYG::onSize(int w, int h)
{
  m_pScreen->onSize(w, h);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, string const & symbolName, yg::EPosition pos, int depth)
{
  m_pScreen->drawSymbol(pt, symbolName.c_str(), pos, depth);
}

void DrawerYG::drawCircle(m2::PointD const & pt, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  double const radius = min(max(pRule->GetRadius() * m_scale, 3.0), 6.0) * m_visualScale;
  unsigned char const alpha = pRule->GetAlpha();
  int const lineC = pRule->GetColor();

  yg::CircleInfo ci(radius,
                    yg::Color::fromXRGB(pRule->GetFillColor(), alpha),
                    lineC != -1,
                    (lineC != -1) ? min(max(pRule->GetWidth() * m_scale * m_visualScale, 1.0), 3.0) : 1.0,
                    yg::Color::fromXRGB(lineC, alpha));

  m_pScreen->drawCircle(pt, ci, pos, depth);
}

void DrawerYG::drawSymbol(m2::PointD const & pt, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  // Use BaseRule::m_id to cache for point draw rule.
  // This rules doesn't mix with other rule-types.

//  uint32_t id = pRule->GetID(m_threadID);

  string name;
  pRule->GetSymbol(name);

/*  if (id == drule::BaseRule::empty_id)
  {
    id = m_pSkin->mapSymbol(name.c_str());

    if (id != drule::BaseRule::empty_id)
      pRule->SetID(m_threadID, id);
    else
    {
      //ASSERT ( false, ("Can't find symbol by id = ", (name)) );
      return;
    }
  }*/

  m_pScreen->drawSymbol(pt, name, pos, depth);
}

void DrawerYG::drawPath(di::PathInfo const & info, di::DrawRule const * rules, size_t count)
{
  // if any rule needs caching - cache as a whole vector
  bool flag = false;
  for (size_t i = 0; i < count; ++i)
  {
    if (rules[i].GetID(m_threadID) == drule::BaseRule::empty_id)
    {
      flag = true;
      break;
    }
  }

  buffer_vector<yg::PenInfo, 8> penInfos(count);
  buffer_vector<uint32_t, 8> styleIDs(count);

  if (flag)
  {
    // collect yg::PenInfo into array and pack them as a whole
    for (size_t i = 0; i < count; ++i)
    {
      rule_ptr_t pRule = rules[i].m_rule;
      vector<double> pattern;
      double offset;
      pRule->GetPattern(pattern, offset);

      for (size_t j = 0; j < pattern.size(); ++j)
        pattern[j] *= m_scale * m_visualScale;

      penInfos[i] = yg::PenInfo(
            yg::Color::fromXRGB(pRule->GetColor(), rules[i].m_transparent ? 100 : pRule->GetAlpha()),
            max(pRule->GetWidth() * m_scale, 1.0) * m_visualScale,
            pattern.empty() ? 0 : &pattern[0], pattern.size(), offset * m_scale);
      styleIDs[i] = m_pSkin->invalidHandle();
    }

    // map array of pens
    if (m_pSkin->mapPenInfo(&penInfos[0], &styleIDs[0], count))
    {
      for (size_t i = 0; i < count; ++i)
        rules[i].SetID(m_threadID, styleIDs[i]);
    }
    else
    {
      LOG(LERROR, ("couldn't successfully pack a sequence of path styles as a whole"));
      return;
    }
  }

  // draw path with array of rules
  for (size_t i = 0; i < count; ++i)
    m_pScreen->drawPath(&info.m_path[0], info.m_path.size(), -info.GetOffset(), rules[i].GetID(m_threadID), rules[i].m_depth);
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
  double const min_text_height_filtered = 2;
  double const min_text_height = 12;          // 8
  //double const min_text_height_mask = 9.99; // 10
}

uint8_t DrawerYG::get_text_font_size(rule_ptr_t pRule) const
{
  double const h = pRule->GetTextHeight() * m_scale;
  return my::rounds(max(h, min_text_height) * m_visualScale);
}

uint8_t DrawerYG::get_pathtext_font_size(rule_ptr_t pRule) const
{
  double const h = pRule->GetTextHeight() * m_scale;
  return my::rounds(max(h, min_text_height) * m_visualScale);
}

bool DrawerYG::filter_text_size(rule_ptr_t pRule) const
{
  return pRule->GetTextHeight() * m_scale <= min_text_height_filtered;
}

void DrawerYG::drawText(m2::PointD const & pt, di::DrawInfo const * pInfo, rule_ptr_t pRule, yg::EPosition pos, int depth)
{
  int const color = pRule->GetFillColor();
  yg::Color textColor(color == -1 ? yg::Color(0, 0, 0, 0) : yg::Color::fromXRGB(color, pRule->GetAlpha()));

  /// to prevent white text on white outline
  if (textColor == yg::Color(255, 255, 255, 255))
    textColor = yg::Color(0, 0, 0, 0);

//  bool isMasked = pRule->GetColor() != -1;
  bool isMasked = true;
  yg::FontDesc fontDesc(get_text_font_size(pRule), textColor, isMasked, yg::Color(255, 255, 255, 255));
  fontDesc.SetRank(pInfo->m_rank);

  if (!filter_text_size(pRule))
    m_pScreen->drawText(
          fontDesc,
          pt,
          pos,
          pInfo->m_name,
          depth,
          true,
          true);
}

bool DrawerYG::drawPathText(di::PathInfo const & info, string const & name, uint8_t fontSize, int depth)
{
//  bool const isMasked = (double(fontSize) / m_visualScale >= min_text_height);

  yg::FontDesc fontDesc(fontSize);

  return m_pScreen->drawPathText( fontDesc,
                                  &info.m_path[0],
                                  info.m_path.size(),
                                  name,
                                  info.GetFullLength(),
                                  info.GetOffset(),
                                  yg::EPosCenter,
                                  depth);
}

void DrawerYG::drawPathNumber(di::PathInfo const & path, di::DrawInfo const * pInfo)
{
  int const textHeight = static_cast<int>(12 * m_visualScale);
  m2::PointD pt;
  double const length = path.GetFullLength();
  if (length >= (pInfo->m_road.size() + 2)*textHeight)
  {
    size_t const count = size_t(length / 1000.0) + 2;

    for (size_t j = 1; j < count; ++j)
    {
      if (path.GetSmPoint(double(j) / double(count), pt))
      {
        yg::FontDesc fontDesc(
          textHeight,
          yg::Color(150, 75, 0, 255),   // brown
          true,
          yg::Color(255, 255, 255, 255));

        m_pScreen->drawText(fontDesc, pt, yg::EPosCenter, pInfo->m_road, yg::maxDepth, true);
      }
    }
  }
}

shared_ptr<yg::gl::Screen> DrawerYG::screen() const
{
  return m_pScreen;
}

double DrawerYG::VisualScale() const
{
  return m_visualScale;
}

void DrawerYG::SetScale(int level)
{
  m_level = level;

#ifdef USE_PROTO_STYLES
  m_scale = 1.0;
#else
  m_scale = scales::GetM2PFactor(level);
#endif
}

void DrawerYG::Draw(di::DrawInfo const * pInfo, di::DrawRule const * rules, size_t count)
{
  buffer_vector<di::DrawRule, 8> pathRules;

  bool const isPath = !pInfo->m_pathes.empty();
  bool const isArea = !pInfo->m_areas.empty();

  // separating path rules from other
  for (unsigned i = 0; i < count; ++i)
  {
    rule_ptr_t pRule = rules[i].m_rule;
    string symbol;
    pRule->GetSymbol(symbol);

    bool const hasSymbol = !symbol.empty();
    bool const isCaption = pRule->GetTextHeight() >= 0.0;

    if (!isCaption && isPath && !hasSymbol && (pRule->GetColor() != -1))
      pathRules.push_back(rules[i]);
  }

  if (!pathRules.empty())
  {
    for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
    {
      drawPath(*i, pathRules.data(), pathRules.size());
    }
  }

  bool isNumber = true;

  for (unsigned i = 0; i < count; ++i)
  {
    rule_ptr_t pRule = rules[i].m_rule;
    int const depth = rules[i].m_depth;

    bool const isCaption = pRule->GetTextHeight() >= 0.0;

    string symbol;
    pRule->GetSymbol(symbol);
    bool const hasSymbol = !symbol.empty();

    bool const isCircle = (pRule->GetRadius() != -1);

    if (!isCaption)
    {
      // path is drawn separately in the code above
      // draw path
      //if (isPath && !isSymbol && (pRule->GetColor() != -1))
      //{
      //  for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
      //    drawPath(*i, pRule, depth);
      //}

      // draw area
      if (isArea)
      {
        bool const isFill = pRule->GetFillColor() != -1;
        bool const hasSym = hasSymbol && ((pRule->GetType() & drule::way) != 0);

        for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
        {
          if (isFill)
            drawArea(i->m_path, pRule, depth);
          else if (hasSym)
            drawSymbol(i->GetCenter() + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
        }
      }

      // draw point symbol
      if (!isPath && !isArea && ((pRule->GetType() & drule::node) != 0))
      {
        if (hasSymbol)
          drawSymbol(pInfo->m_point + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
        if (isCircle)
          drawCircle(pInfo->m_point + m2::PointD(0, 2 * m_visualScale), pRule, yg::EPosUnder, depth);
      }
    }
    else
    {
      if (!pInfo->m_name.empty())
      {
        bool isN = ((pRule->GetType() & drule::way) != 0);

        // draw area text
        if (isArea/* && isN*/)
        {
          for (list<di::AreaInfo>::const_iterator i = pInfo->m_areas.begin(); i != pInfo->m_areas.end(); ++i)
            drawText(i->GetCenter(), pInfo, pRule, yg::EPosAbove, depth);
        }

        // draw way name
        if (isPath && !isArea && isN && !filter_text_size(pRule))
        {
          for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
          {
            if (filter_text_size(pRule))
              continue;

            drawPathText(*i, pInfo->m_name, get_pathtext_font_size(pRule), depth);
          }
        }

        // draw point text
        isN = ((pRule->GetType() & drule::node) != 0);
        if (!isPath && !isArea && isN)
          drawText(pInfo->m_point, pInfo, pRule, yg::EPosAbove, depth);
      }
    }
  }

  // draw road numbers
  if (isNumber && isPath && !pInfo->m_road.empty() && m_level >= 12)
  {
    for (list<di::PathInfo>::const_iterator i = pInfo->m_pathes.begin(); i != pInfo->m_pathes.end(); ++i)
      drawPathNumber(*i, pInfo);
  }
}
