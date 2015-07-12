#include "cpu_drawer.hpp"
#include "proto_to_styles.hpp"

#include "geometry/tree4d.hpp"
#include "geometry/transformations.hpp"

#include "graphics/text_path.hpp"

#include "base/macros.hpp"
#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"

namespace
{

void CorrectFont(graphics::FontDesc & font)
{
  font.m_size = font.m_size * 0.75;
}

strings::UniString PreProcessText(string const & text)
{
  strings::UniString logText = strings::MakeUniString(text);
  strings::UniString visText = graphics::GlyphCache::log2vis(logText);
  char const * delims = " \n\t";

  if (logText != visText)
    return visText;

  size_t count = visText.size();
  if (count > 15)
  {
    // split on two parts
    typedef strings::UniString::const_iterator TIter;
    TIter const iBegin = visText.begin();
    TIter const iMiddle = iBegin + count / 2;
    TIter const iEnd = visText.end();

    size_t const delimsSize = strlen(delims);

    // find next delimeter after middle [m, e)
    TIter iNext = find_first_of(iMiddle, iEnd,
                                delims, delims + delimsSize);

    // find last delimeter before middle [b, m)
    TIter iPrev = find_first_of(reverse_iterator<TIter>(iMiddle),
                                reverse_iterator<TIter>(iBegin),
                                delims, delims + delimsSize).base();
    // don't do split like this:
    //     xxxx
    // xxxxxxxxxxxx
    if (4 * distance(iBegin, iPrev) <= count)
      iPrev = visText.end();
    else
      --iPrev;

    // get closest delimiter to the middle
    if (iNext == iEnd ||
        (iPrev != iEnd && distance(iPrev, iMiddle) < distance(iMiddle, iNext)))
    {
      iNext = iPrev;
    }

    // split string on 2 parts
    if (iNext != visText.end())
      ASSERT_NOT_EQUAL(iNext, iEnd, ());
      visText.insert(iNext, '\n');
  }

  return visText;
}

template<typename TInfo>
FeatureID const & InsertImpl(FeatureID const & id, map<FeatureID, TInfo> & map, TInfo const & info)
{
  if (map.find(id) == map.end())
    map.insert(make_pair(id, info));

  return id;
}

template<typename TInfo>
TInfo const & GetInfo(FeatureID const & id, map<FeatureID, TInfo> & m)
{
  ASSERT(m.find(id) != m.end(), ());
  return m[id];
}

}

class CPUDrawer::CPUOverlayTree
{
  struct OverlayWrapperTraits
  {
    static m2::RectD const LimitRect(CPUDrawer::OverlayWrapper const * elem)
    {
      m2::RectD result;
      for_each(elem->m_rects.begin(), elem->m_rects.end(), [&result](m2::RectD const & r)
      {
        result.Add(r);
      });

      return result;
    }
  };

  class DoPreciseIntersect
  {
    CPUDrawer::OverlayWrapper const * m_elem;
    bool m_isIntersect;

  public:
    DoPreciseIntersect(CPUDrawer::OverlayWrapper const * oe)
      : m_elem(oe), m_isIntersect(false)
    {
    }

    void operator()(CPUDrawer::OverlayWrapper const * e)
    {
      if (m_isIntersect)
        return;

      if (m_elem == e)
        return;

      for (m2::RectD const & elemR : m_elem->m_rects)
        for (m2::RectD const & inputR : e->m_rects)
        {
          m_isIntersect = elemR.IsIntersect(inputR);
          if (m_isIntersect)
            return;
        }
    }

    bool IsIntersect() const { return m_isIntersect; }
  };

public:
  template <typename Fn>
  void forEach(Fn fn)
  {
    m_tree.ForEach(fn);
  }

  void replaceOverlayElement(CPUDrawer::OverlayWrapper const * oe)
  {
    DoPreciseIntersect fn(oe);
    m_tree.ForEachInRect(OverlayWrapperTraits::LimitRect(oe), ref(fn));

    if (fn.IsIntersect())
    {
      m_tree.ReplaceAllInRect(oe, [](CPUDrawer::OverlayWrapper const * l,
                                     CPUDrawer::OverlayWrapper const * r)
      {
        return l->m_rules[0]->m_drawRule.m_depth > r->m_rules[0]->m_drawRule.m_depth;
      });
    }
    else
      m_tree.Add(oe);
  }

private:
  m4::Tree<CPUDrawer::OverlayWrapper const *, OverlayWrapperTraits> m_tree;
};

CPUDrawer::CPUDrawer(Params const & params)
  : TBase(params)
  , m_renderer(new SoftwareRenderer(params.m_glyphCacheParams, params.m_density))
  , m_generationCounter(0)
{
}

void CPUDrawer::BeginFrame(uint32_t width, uint32_t height)
{
  m_renderer->BeginFrame(width, height);
}

void CPUDrawer::Flush()
{
  Render();
}

void CPUDrawer::DrawMyPosition(m2::PointD const & myPxPotision)
{
  m_renderer->DrawSymbol(myPxPotision, graphics::EPosCenter,
                         graphics::Icon::Info("watch-my-position"));
}

void CPUDrawer::DrawSearchResult(m2::PointD const & pxPosition)
{
  m_renderer->DrawSymbol(pxPosition, graphics::EPosAbove,
                         graphics::Icon::Info("watch-search-result"));
}

void CPUDrawer::DrawSearchArrow(double azimut)
{
  PathWrapper path;

  {
    PathParams firstPass;
    firstPass.m_isFill = true;
    firstPass.m_fillColor = agg::rgba8(0x4C, 0xAF, 0x50);
    firstPass.m_isStroke = true;
    firstPass.m_strokeColor = agg::rgba8(0x0, 0x0, 0x0, 0x1A);
    firstPass.m_isEventOdd = true;
    firstPass.m_strokeWidth = 6;

    path.AddParams(firstPass);
  }

  {
    PathParams secondPass;
    secondPass.m_isFill = false;
    secondPass.m_fillColor = agg::rgba8(0, 0, 0);
    secondPass.m_isStroke = true;
    secondPass.m_strokeColor = agg::rgba8(0xFF, 0xFF, 0xFF);
    secondPass.m_strokeWidth = 4;

    path.AddParams(secondPass);
  }

  path.MoveTo(m2::PointD(349.05025, 55.3532299));
  path.CurveTo(m2::PointD(352.214211, 55.3532299),
               m2::PointD(354.779108, 52.671954),
               m2::PointD(354.779108, 49.3644327));
  path.CurveTo(m2::PointD(354.779108, 46.0569113),
               m2::PointD(352.214211, 43.3756355),
               m2::PointD(349.05025,  43.3756355));
  path.CurveTo(m2::PointD(345.886289, 43.3756355),
               m2::PointD(343.321392, 46.0569113),
               m2::PointD(343.321392, 49.3644327));
  path.CurveTo(m2::PointD(343.321392, 52.671954),
               m2::PointD(345.886289, 55.3532299),
               m2::PointD(349.05025,  55.3532299));
  path.ClosePath();
  path.MoveTo(m2::PointD(333.752104, 42.7175982));

  path.CurveTo(m2::PointD(332.709155, 43.0810222),
               m2::PointD(332.359503, 42.6413443),
               m2::PointD(332.978094, 41.7252445));
  path.LineTo(m2::PointD(347.935833, 19.5736386));
  path.CurveTo(m2::PointD(348.551309, 18.6621528),
               m2::PointD(349.546076, 18.6575388),
               m2::PointD(350.164667, 19.5736386));
  path.LineTo(m2::PointD(365.122406, 41.7252445));

  path.CurveTo(m2::PointD(365.737881, 42.6367303),
               m2::PointD(365.384538, 43.0786501),
               m2::PointD(364.348396, 42.7175982));
  path.LineTo(m2::PointD(349.05025, 37.3868384));
  path.LineTo(m2::PointD(333.752104, 42.7175982));
  path.ClosePath();

  m2::RectD symbolRect;
  path.BoundingRect(symbolRect);
  m2::PointD symbolCenter = symbolRect.Center();

  m2::RectD frameRect = m_renderer->FrameRect();
  m2::PointD halfSize(0.5 * frameRect.SizeX(), 0.5 * frameRect.SizeY());
  m2::PointD frameCenter = frameRect.Center();

  double length = halfSize.x;
  double azIn2Pi = ang::AngleIn2PI(azimut);

  if (azIn2Pi > math::pi4 && azIn2Pi < math::pi2 + math::pi4)
  {
    length = halfSize.y;
    azIn2Pi = fabs(math::pi2 - azimut);
  }
  else if (azIn2Pi > math::pi + math::pi4 && azIn2Pi < math::pi + math::pi2 + math::pi4)
  {
    length = halfSize.y;
    azIn2Pi = fabs(math::pi2 - azimut);
  }

  double cosa = cos(azIn2Pi);
  if (!my::AlmostEqualULPs(cosa, 0.0))
    length = length / fabs(cosa);

  m2::PointD offsetPoint(length - 0.65 * symbolRect.SizeY(), 0.0);
  offsetPoint.Rotate(-azimut);

  math::Matrix<double, 3, 3> m = math::Shift(math::Rotate(math::Scale(math::Shift(math::Identity<double, 3>(),
                                                                                  -symbolCenter),
                                                                      1.5, 1.5),
                                                          math::pi2 - azimut),
                                             frameCenter + offsetPoint);

  m_renderer->DrawPath(path, m);
}

void CPUDrawer::EndFrame(FrameImage & image)
{
  m_renderer->EndFrame(image);
  m_stylers.clear();
  m_areasGeometry.clear();
  m_pathGeometry.clear();
  m_roadNames.clear();

  m_areaPathShapes.clear();
  m_pointShapes.clear();
  m_textShapes.clear();
  m_pathTextShapes.clear();
  m_overlayList.clear();
}

void CPUDrawer::DrawSymbol(m2::PointD const & pt, graphics::EPosition pos, di::DrawRule const & rule)
{
  m_pointShapes.emplace_back(pt, pos, TYPE_SYMBOL, rule, GetGeneration());
  OverlayWrapper & overlay = AddOverlay(&m_pointShapes.back());

  graphics::Icon::Info icon;
  ConvertStyle(overlay.m_rules[0]->m_drawRule.m_rule->GetSymbol(), icon);

  m2::RectD rect;
  m_renderer->CalculateSymbolMetric(pt, pos, icon, rect);
  overlay.m_rects.push_back(rect);
}

void CPUDrawer::DrawCircle(m2::PointD const & pt, graphics::EPosition pos, di::DrawRule const & rule)
{
  m_pointShapes.emplace_back(pt, pos, TYPE_CIRCLE, rule, GetGeneration());
  OverlayWrapper & overlay = AddOverlay(&m_pointShapes.back());

  graphics::Circle::Info info;
  ConvertStyle(overlay.m_rules[0]->m_drawRule.m_rule->GetCircle(), VisualScale(), info);

  m2::RectD rect;
  m_renderer->CalculateCircleMetric(pt, pos, info, rect);
  overlay.m_rects.push_back(rect);
}

void CPUDrawer::DrawCircledSymbol(m2::PointD const & pt, graphics::EPosition pos,
                                  di::DrawRule const & symbolRule, di::DrawRule const & circleRule)
{
  m_pointShapes.emplace_back(pt, pos, TYPE_CIRCLE, circleRule, GetGeneration());
  BaseShape const * circleShape = &m_pointShapes.back();
  m_pointShapes.emplace_back(pt, pos, TYPE_SYMBOL, symbolRule, GetGeneration());
  BaseShape const * symbolShape = &m_pointShapes.back();

  OverlayWrapper & overlay = AddOverlay(circleShape, symbolShape);

  m2::RectD rect;

  graphics::Circle::Info circleInfo;
  ConvertStyle(overlay.m_rules[0]->m_drawRule.m_rule->GetCircle(), VisualScale(), circleInfo);
  m_renderer->CalculateCircleMetric(pt, pos, circleInfo, rect);
  overlay.m_rects.push_back(rect);

  graphics::Icon::Info symbolInfo;
  ConvertStyle(overlay.m_rules[1]->m_drawRule.m_rule->GetSymbol(), symbolInfo);
  m_renderer->CalculateSymbolMetric(pt, pos, symbolInfo, rect);
  overlay.m_rects.back().Add(rect);
}

void CPUDrawer::DrawPath(di::PathInfo const & path, di::DrawRule const * rules, size_t count)
{
  FeatureID const & id = Insert(path);
  for (size_t i = 0; i < count; ++i)
    m_areaPathShapes.emplace_back(id, rules[i], GetGeneration(), TYPE_PATH);
}

void CPUDrawer::DrawArea(di::AreaInfo const & area, di::DrawRule const & rule)
{
  m_areaPathShapes.emplace_back(Insert(area), rule, GetGeneration(), TYPE_AREA);
}

void CPUDrawer::DrawText(m2::PointD const & pt, graphics::EPosition pos,
                         di::FeatureStyler const & fs, di::DrawRule const & rule)
{
  m_textShapes.emplace_back(pt, pos, Insert(fs), rule, GetGeneration(), TYPE_TEXT);
  TextShape const * shape = &m_textShapes.back();
  OverlayWrapper & overlay = AddOverlay(shape);

  TTextRendererCall callback = [this, &overlay](m2::PointD const & pt, graphics::EPosition anchor,
                                                graphics::FontDesc const & primFont,
                                                graphics::FontDesc const & secFont,
                                                strings::UniString const & primText,
                                                strings::UniString const & secText)
  {
    m2::RectD rect;
    if (secText.empty())
      m_renderer->CalculateTextMetric(pt, anchor, primFont, primText, rect);
    else
      m_renderer->CalculateTextMetric(pt, anchor, primFont, secFont, primText, secText, rect);
    overlay.m_rects.push_back(rect);
  };

  CallTextRendererFn(shape, callback);
}

void CPUDrawer::DrawPathText(di::PathInfo const & info, di::FeatureStyler const & fs, di::DrawRule const & rule)
{
  FeatureID const & geomID = Insert(info);
  FeatureID const & styleID = Insert(fs);
  ASSERT(geomID == styleID, ());
  UNUSED_VALUE(styleID);
  m_pathTextShapes.emplace_back(geomID, rule, GetGeneration(), TYPE_PATH_TEXT);
  ComplexShape const * shape = &m_pathTextShapes.back();
  OverlayWrapper & overlay = AddOverlay(shape);

  TPathTextRendererCall callback = [this, &overlay](di::PathInfo const & geometry,
                                                    graphics::FontDesc const & font,
                                                    strings::UniString const & text)
  {
    m_renderer->CalculateTextMetric(geometry, font, text, overlay.m_rects);
  };

  CallTextRendererFn(shape, callback);
}

void CPUDrawer::DrawPathNumber(di::PathInfo const & path, di::FeatureStyler const & fs)
{
  FeatureID const & id = Insert(fs.m_refText);
  GenerateRoadNumbers(path, fs, [this, &id](m2::PointD const & pt, graphics::FontDesc const & font, string const & /*text*/)
  {
    m_roadNumberFont = font;
    m_textShapes.emplace_back(pt, graphics::EPosCenter, id, di::DrawRule(), GetGeneration(), TYPE_ROAD_NUMBER);
    TextShape const * shape = &m_textShapes.back();
    OverlayWrapper & overlay = AddOverlay(shape);

    TRoadNumberRendererCall callback = [this, &overlay](m2::PointD const & pt, graphics::EPosition anchor,
                                                        graphics::FontDesc const & font, strings::UniString const & text)
    {
      m2::RectD rect;
      m_renderer->CalculateTextMetric(pt, anchor, font, text, rect);
      overlay.m_rects.push_back(rect);
    };

    CallTextRendererFn(shape, callback);
  });
}

FeatureID const & CPUDrawer::Insert(di::PathInfo const & info)
{
  return InsertImpl(GetCurrentFeatureID(), m_pathGeometry, info);
}

FeatureID const & CPUDrawer::Insert(di::AreaInfo const & info)
{
  return InsertImpl(GetCurrentFeatureID(), m_areasGeometry, info);
}

FeatureID const & CPUDrawer::Insert(di::FeatureStyler const & styler)
{
  return InsertImpl(GetCurrentFeatureID(), m_stylers, styler);
}

FeatureID const & CPUDrawer::Insert(string const & text)
{
  return InsertImpl(GetCurrentFeatureID(), m_roadNames, text);
}

void CPUDrawer::Render()
{
  m_areaPathShapes.sort([](BaseShape const & lsh, BaseShape const & rsh)
  {
    if (lsh.m_drawRule.m_depth == rsh.m_drawRule.m_depth)
      return lsh.m_generation < rsh.m_generation;

    return lsh.m_drawRule.m_depth < rsh.m_drawRule.m_depth;
  });

  auto const renderFn = [this](BaseShape const & s)
  {
    BaseShape const * shape = &s;
    switch (shape->m_type)
    {
    case TYPE_SYMBOL:
      DrawSymbol(static_cast<PointShape const *>(shape));
      break;
    case TYPE_CIRCLE:
      DrawCircle(static_cast<PointShape const *>(shape));
      break;
    case TYPE_PATH:
      DrawPath(static_cast<ComplexShape const *>(shape));
      break;
    case TYPE_AREA:
      DrawArea(static_cast<ComplexShape const *>(shape));
      break;
    case TYPE_PATH_TEXT:
      DrawPathText(static_cast<ComplexShape const *>(shape));
      break;
    case TYPE_TEXT:
      DrawText(static_cast<TextShape const *>(shape));
      break;
    case TYPE_ROAD_NUMBER:
      DrawRoadNumber(static_cast<TextShape const *>(shape));
      break;
    default:
      ASSERT(false, ());
      break;
    }
  };

  for_each(m_areaPathShapes.begin(), m_areaPathShapes.end(), renderFn);
  CPUOverlayTree tree;
  for_each(m_overlayList.begin(), m_overlayList.end(), [&tree](OverlayWrapper const & oe)
  {
    tree.replaceOverlayElement(&oe);
  });

  tree.forEach([&renderFn](OverlayWrapper const * oe)
  {
    ASSERT(oe->m_rules[0] != nullptr, ());
    renderFn(*oe->m_rules[0]);
    if (oe->m_ruleCount > 1)
    {
      ASSERT(oe->m_rules[1] != nullptr, ());
      renderFn(*oe->m_rules[1]);
    }
  });
}

void CPUDrawer::DrawSymbol(PointShape const * shape)
{
  ASSERT(shape->m_type == TYPE_SYMBOL, ());
  ASSERT(shape->m_drawRule.m_rule != nullptr, ());
  ASSERT(shape->m_drawRule.m_rule->GetSymbol() != nullptr, ());

  graphics::Icon::Info info;
  ConvertStyle(shape->m_drawRule.m_rule->GetSymbol(), info);
  m_renderer->DrawSymbol(shape->m_position, shape->m_anchor, info);
}

void CPUDrawer::DrawCircle(PointShape const * shape)
{
  ASSERT(shape->m_type == TYPE_CIRCLE, ());
  ASSERT(shape->m_drawRule.m_rule != nullptr, ());
  ASSERT(shape->m_drawRule.m_rule->GetCircle() != nullptr, ());

  graphics::Circle::Info info;
  ConvertStyle(shape->m_drawRule.m_rule->GetCircle(), VisualScale(), info);
  m_renderer->DrawCircle(shape->m_position, shape->m_anchor, info);
}

void CPUDrawer::DrawPath(ComplexShape const * shape)
{
  ASSERT(shape->m_type == TYPE_PATH, ());
  ASSERT(shape->m_drawRule.m_rule != nullptr, ());
  ASSERT(shape->m_drawRule.m_rule->GetLine() != nullptr, ());

  graphics::Pen::Info info;
  ConvertStyle(shape->m_drawRule.m_rule->GetLine(), VisualScale(), info);

  m_renderer->DrawPath(GetInfo(shape->m_geomID, m_pathGeometry), info);
}

void CPUDrawer::DrawArea(ComplexShape const * shape)
{
  ASSERT(shape->m_type == TYPE_AREA, ());
  ASSERT(shape->m_drawRule.m_rule != nullptr, ());
  ASSERT(shape->m_drawRule.m_rule->GetArea() != nullptr, ());

  graphics::Brush::Info info;
  ConvertStyle(shape->m_drawRule.m_rule->GetArea(), info);

  m_renderer->DrawArea(GetInfo(shape->m_geomID, m_areasGeometry), info);
}

void CPUDrawer::DrawPathText(ComplexShape const * shape)
{
  TPathTextRendererCall callback = [this](di::PathInfo const & geometry,
                                          graphics::FontDesc const & font,
                                          strings::UniString const & text)
  {
    m_renderer->DrawPathText(geometry, font, text);
  };

  CallTextRendererFn(shape, callback);
}

void CPUDrawer::DrawRoadNumber(TextShape const * shape)
{
  TRoadNumberRendererCall callback = [this](m2::PointD const & pt, graphics::EPosition pos,
                                            graphics::FontDesc const & font, strings::UniString const & text)
  {
    m_renderer->DrawText(pt, pos, font, text);
  };

  CorrectFont(m_roadNumberFont);
  CallTextRendererFn(shape, callback);
}

void CPUDrawer::DrawText(TextShape const * shape)
{
  TTextRendererCall callback = [this](m2::PointD const & pt, graphics::EPosition anchor,
                                      graphics::FontDesc const & primFont, graphics::FontDesc const & secFont,
                                      strings::UniString const & primText, strings::UniString const & secText)
  {
    if (secText.empty())
      m_renderer->DrawText(pt, anchor, primFont, primText);
    else
      m_renderer->DrawText(pt, anchor, primFont, secFont, primText, secText);
  };
  CallTextRendererFn(shape, callback);
}

CPUDrawer::OverlayWrapper & CPUDrawer::AddOverlay(BaseShape const * shape1, CPUDrawer::BaseShape const * shape2)
{
  BaseShape const * shapes[] = { shape1, shape2 };
  m_overlayList.emplace_back(shapes, shape2 == nullptr ? 1 : 2);
  return m_overlayList.back();
}

void CPUDrawer::CallTextRendererFn(TextShape const * shape, TTextRendererCall const & fn)
{
  ASSERT(shape->m_type == TYPE_TEXT, ());
  ASSERT(shape->m_drawRule.m_rule != nullptr, ());
  ASSERT(shape->m_drawRule.m_rule->GetCaption(0) != nullptr, ());
  di::FeatureStyler const & fs = GetInfo(shape->m_geomID, m_stylers);

  CaptionDefProto const * primCaption = shape->m_drawRule.m_rule->GetCaption(0);
  CaptionDefProto const * secCaption = shape->m_drawRule.m_rule->GetCaption(1);

  graphics::FontDesc primFont, secFont;
  m2::PointD primOffset, secOffset;
  ConvertStyle(primCaption, VisualScale(), primFont, primOffset);
  primFont.SetRank(fs.m_popRank);
  CorrectFont(primFont);
  if (secCaption != nullptr)
  {
    ConvertStyle(secCaption, VisualScale(), secFont, secOffset);
    secFont.SetRank(fs.m_popRank);
    CorrectFont(secFont);
  }

  fn(shape->m_position + primOffset, shape->m_anchor, primFont, secFont,
     PreProcessText(fs.m_primaryText), PreProcessText(fs.m_secondaryText));
}

void CPUDrawer::CallTextRendererFn(TextShape const * shape, TRoadNumberRendererCall const & fn)
{
  string const & text = GetInfo(shape->m_geomID, m_roadNames);

  fn(shape->m_position, shape->m_anchor, m_roadNumberFont, graphics::GlyphCache::log2vis(strings::MakeUniString(text)));
}

void CPUDrawer::CallTextRendererFn(ComplexShape const * shape, TPathTextRendererCall const & fn)
{
  ASSERT(shape->m_type == TYPE_PATH_TEXT, ());

  di::PathInfo const & path = GetInfo(shape->m_geomID, m_pathGeometry);
  di::FeatureStyler const & styler = GetInfo(shape->m_geomID, m_stylers);

  if (styler.m_offsets.empty())
    return;

  graphics::FontDesc font;
  m2::PointD offset;
  ConvertStyle(shape->m_drawRule.m_rule->GetCaption(0), VisualScale(), font, offset);
  CorrectFont(font);

  fn(path, font, strings::MakeUniString(styler.GetPathName()));
}
