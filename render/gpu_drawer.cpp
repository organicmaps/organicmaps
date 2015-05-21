#include "gpu_drawer.hpp"
#include "feature_styler.hpp"
#include "proto_to_styles.hpp"
#include "path_info.hpp"
#include "area_info.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/feature_decl.hpp"

#include "graphics/screen.hpp"

namespace
{
  graphics::OverlayElement::UserInfo ToUserInfo(FeatureID const & id)
  {
    graphics::OverlayElement::UserInfo info;
    info.m_offset = id.m_offset;
    info.m_mwmID = id.m_mwmId;
    return info;
  }
}

GPUDrawer::GPUDrawer(Params const & params)
  : TBase(params)
  , m_pScreen(new graphics::Screen(params.m_screenParams))
{
  for (unsigned i = 0; i < m_pScreen->pipelinesCount(); ++i)
    m_pScreen->addClearPageFn(i, bind(&GPUDrawer::ClearResourceCache, ThreadSlot(), i), 0);
}

graphics::Screen * GPUDrawer::Screen() const
{
  return m_pScreen.get();
}

void GPUDrawer::BeginFrame()
{
  m_pScreen->beginFrame();
}

void GPUDrawer::EndFrame()
{
  m_pScreen->endFrame();
}

void GPUDrawer::OnSize(int w, int h)
{
  m_pScreen->onSize(w, h);
}

graphics::GlyphCache * GPUDrawer::GetGlyphCache()
{
  return Screen()->glyphCache();
}

int GPUDrawer::ThreadSlot() const
{
  return m_pScreen->threadSlot();
}

void GPUDrawer::DrawCircle(m2::PointD const & pt,
                           graphics::EPosition pos,
                           di::DrawRule const & rule)
{
  graphics::Circle::Info ci;
  ConvertStyle(rule.m_rule->GetCircle(), VisualScale(), ci);

  graphics::CircleElement::Params params;

  params.m_depth = rule.m_depth;
  params.m_position = pos;
  params.m_pivot = pt;
  params.m_ci = ci;
  params.m_userInfo = ToUserInfo(GetCurrentFeatureID());

  m_pScreen->drawCircle(params);
}

void GPUDrawer::DrawSymbol(m2::PointD const & pt,
                           graphics::EPosition pos,
                           di::DrawRule const & rule)
{
  graphics::Icon::Info info;
  ConvertStyle(rule.m_rule->GetSymbol(), info);

  graphics::SymbolElement::Params params;
  params.m_depth = rule.m_depth;
  params.m_position = pos;
  params.m_pivot = pt;
  params.m_info = info;
  params.m_renderer = m_pScreen.get();
  params.m_userInfo = ToUserInfo(GetCurrentFeatureID());

  m_pScreen->drawSymbol(params);
}

void GPUDrawer::DrawCircledSymbol(m2::PointD const & pt,
                                  graphics::EPosition pos,
                                  di::DrawRule const & symbolRule,
                                  di::DrawRule const & circleRule)
{
  graphics::Icon::Info info;
  ConvertStyle(symbolRule.m_rule->GetSymbol(), info);

  graphics::Circle::Info ci;
  ConvertStyle(circleRule.m_rule->GetCircle(), VisualScale(), ci);

  graphics::SymbolElement::Params symParams;
  symParams.m_depth = symbolRule.m_depth;
  symParams.m_position = pos;
  symParams.m_pivot = pt;
  symParams.m_info = info;
  symParams.m_renderer = m_pScreen.get();
  symParams.m_userInfo = ToUserInfo(GetCurrentFeatureID());

  graphics::CircleElement::Params circleParams;
  circleParams.m_depth = circleRule.m_depth;
  circleParams.m_position = pos;
  circleParams.m_pivot = pt;
  circleParams.m_ci = ci;
  circleParams.m_userInfo = ToUserInfo(GetCurrentFeatureID());

  m_pScreen->drawCircledSymbol(symParams, circleParams);
}

void GPUDrawer::DrawPath(di::PathInfo const & path, di::DrawRule const * rules, size_t count)
{
  // if any rule needs caching - cache as a whole vector
  bool flag = false;
  for (size_t i = 0; i < count; ++i)
  {
    if (rules[i].GetID(m_pScreen->threadSlot()) == drule::BaseRule::empty_id)
    {
      flag = true;
      break;
    }
  }

  buffer_vector<graphics::Pen::Info, 8> penInfos(count);
  buffer_vector<graphics::Resource::Info const*, 8> infos(count);
  buffer_vector<uint32_t, 8> resIDs(count);

  if (flag)
  {
    // collect graphics::PenInfo into array and pack them as a whole
    for (size_t i = 0; i < count; ++i)
    {
      ConvertStyle(rules[i].m_rule->GetLine(), VisualScale(), penInfos[i]);

      infos[i] = &penInfos[i];

      resIDs[i] = m_pScreen->invalidHandle();
    }

    // map array of pens
    if (m_pScreen->mapInfo(&infos[0], &resIDs[0], count))
    {
      for (size_t i = 0; i < count; ++i)
        rules[i].SetID(ThreadSlot(), resIDs[i]);
    }
    else
    {
      buffer_vector<m2::PointU, 8> rects;
      for (unsigned i = 0; i < count; ++i)
        rects.push_back(infos[i]->resourceSize());
      LOG(LERROR, ("couldn't successfully pack a sequence of path styles as a whole:" , rects));

      return;
    }
  }

  // draw path with array of rules
  for (size_t i = 0; i < count; ++i)
    m_pScreen->drawPath(&path.m_path[0],
                        path.m_path.size(),
                        -path.GetOffset(),
                        rules[i].GetID(ThreadSlot()),
                        rules[i].m_depth);
}

void GPUDrawer::DrawArea(di::AreaInfo const & area, di::DrawRule const & rule)
{
  // DO NOT cache 'id' in pRule, because one rule can use in drawPath and drawArea.
  // Leave CBaseRule::m_id for drawPath. mapColor working fast enough.

  graphics::Brush::Info info;
  ConvertStyle(rule.m_rule->GetArea(), info);

  uint32_t const id = m_pScreen->mapInfo(info);
  ASSERT ( id != -1, () );

  m_pScreen->drawTrianglesList(&area.m_path[0], area.m_path.size(), id, rule.m_depth);
}

void GPUDrawer::DrawText(m2::PointD const & pt,
                         graphics::EPosition pos,
                         di::FeatureStyler const & fs,
                         di::DrawRule const & rule)
{
  graphics::FontDesc primaryFont;
  m2::PointD primaryOffset;
  ConvertStyle(rule.m_rule->GetCaption(0), VisualScale(), primaryFont, primaryOffset);
  primaryFont.SetRank(fs.m_popRank);

  graphics::FontDesc secondaryFont;
  m2::PointD secondaryOffset;
  if (rule.m_rule->GetCaption(1))
  {
    ConvertStyle(rule.m_rule->GetCaption(1), VisualScale(), secondaryFont, secondaryOffset);
    secondaryFont.SetRank(fs.m_popRank);
  }

  graphics::StraightTextElement::Params params;
  params.m_depth = rule.m_depth;
  params.m_fontDesc = primaryFont;
  params.m_auxFontDesc = secondaryFont;
  params.m_offset = primaryOffset;
  params.m_log2vis = true;
  params.m_pivot = pt;
  params.m_position = pos;
  params.m_logText = strings::MakeUniString(fs.m_primaryText);
  params.m_auxLogText = strings::MakeUniString(fs.m_secondaryText);
  params.m_doSplit = true;
  params.m_useAllParts = false;
  params.m_userInfo = ToUserInfo(GetCurrentFeatureID());

  m_pScreen->drawTextEx(params);
}

void GPUDrawer::DrawPathText(di::PathInfo const & path,
                             di::FeatureStyler const & fs,
                             di::DrawRule const & rule)
{
  graphics::FontDesc font;
  m2::PointD offset;
  ConvertStyle(rule.m_rule->GetCaption(0), VisualScale(), font, offset);

  if (fs.m_offsets.empty())
    return;

  m_pScreen->drawPathText(font,
                          &path.m_path[0],
                          path.m_path.size(),
                          fs.GetPathName(),
                          path.GetFullLength(),
                          path.GetOffset(),
                          &fs.m_offsets[0],
                          fs.m_offsets.size(),
                          rule.m_depth);
}

void GPUDrawer::DrawPathNumber(di::PathInfo const & path,
                               di::FeatureStyler const & fs)
{
  GenerateRoadNumbers(path, fs, [this](m2::PointD const & pt, graphics::FontDesc const & font, string const & text)
  {
    m_pScreen->drawText(font, pt, graphics::EPosCenter, text, 0, true /*log2vis*/);
  });
}

namespace
{
  struct DoMakeInvalidRule
  {
    size_t m_threadSlot;
    uint32_t m_pipelineIDMask;

    DoMakeInvalidRule(size_t threadSlot, uint8_t pipelineID)
      : m_threadSlot(threadSlot), m_pipelineIDMask(pipelineID << 24)
    {}

    void operator() (int, int, int, drule::BaseRule * p)
    {
      if ((p->GetID(m_threadSlot) & 0xFF000000) == m_pipelineIDMask)
        p->MakeEmptyID(m_threadSlot);
    }
  };
}


void GPUDrawer::ClearResourceCache(size_t threadSlot, uint8_t pipelineID)
{
  drule::rules().ForEachRule(DoMakeInvalidRule(threadSlot, pipelineID));
}


graphics::Screen * GPUDrawer::GetScreen(Drawer * drawer)
{
  ASSERT(dynamic_cast<GPUDrawer *>(drawer) != nullptr, ());
  return static_cast<GPUDrawer *>(drawer)->Screen();
}
