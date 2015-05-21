#pragma once

#include "drawer.hpp"
#include "feature_styler.hpp"
#include "software_renderer.hpp"

#include "std/list.hpp"
#include "std/unique_ptr.hpp"

class CPUDrawer : public Drawer
{
  using TBase = Drawer;

public:
  struct Params : Drawer::Params
  {
    Params(graphics::GlyphCache::Params const & p)
      : m_glyphCacheParams(p)
    {
    }

    graphics::GlyphCache::Params m_glyphCacheParams;
    graphics::EDensity m_density;
  };

  CPUDrawer(Params const & params);

  void BeginFrame(uint32_t width, uint32_t height);
    void Flush();
    void DrawMyPosition(m2::PointD const & myPxPotision);
    void DrawSearchResult(m2::PointD const & pxPosition);
    void DrawSearchArrow(double azimut);
  void EndFrame(FrameImage & image);

  graphics::GlyphCache * GetGlyphCache() { return m_renderer->GetGlyphCache(); }

protected:
  void DrawSymbol(m2::PointD const & pt, graphics::EPosition pos, di::DrawRule const & rule) override;
  void DrawCircle(m2::PointD const & pt, graphics::EPosition pos, di::DrawRule const & rule) override;
  void DrawCircledSymbol(m2::PointD const & pt, graphics::EPosition pos,
                         di::DrawRule const & symbolRule, di::DrawRule const & circleRule) override;
  void DrawPath(di::PathInfo const & path, di::DrawRule const * rules, size_t count) override;
  void DrawArea(di::AreaInfo const & area, di::DrawRule const & rule) override;
  void DrawText(m2::PointD const & pt, graphics::EPosition pos,
                di::FeatureStyler const & fs, di::DrawRule const & rule) override;
  void DrawPathText(di::PathInfo const & info, di::FeatureStyler const & fs, di::DrawRule const & rule) override;
  void DrawPathNumber(di::PathInfo const & path, di::FeatureStyler const & fs) override;

  int GetGeneration() { return m_generationCounter++; }

  FeatureID const & Insert(di::PathInfo const & info);
  FeatureID const & Insert(di::AreaInfo const & info);
  FeatureID const & Insert(di::FeatureStyler const & styler);
  FeatureID const & Insert(string const & text);

private:
  void Render();

private:
  unique_ptr<SoftwareRenderer> m_renderer;
  int m_generationCounter;

  enum EShapeType
  {
    TYPE_INVALID,
    TYPE_SYMBOL,
    TYPE_CIRCLE,
    TYPE_AREA,
    TYPE_PATH,
    TYPE_PATH_TEXT,
    TYPE_TEXT,
    TYPE_ROAD_NUMBER
  };

  struct BaseShape
  {
    BaseShape(di::DrawRule const & rule, int generation, EShapeType type)
      : m_drawRule(rule)
      , m_generation(generation)
      , m_type(type)
    {
    }

    di::DrawRule m_drawRule;
    int m_generation = 0;
    EShapeType m_type = TYPE_INVALID;
  };

  struct PointShape : BaseShape
  {
    PointShape(m2::PointD const & pt, graphics::EPosition anchor, EShapeType type,
               di::DrawRule const & rule, int generation)
      : BaseShape(rule, generation, type)
      , m_position(pt)
      , m_anchor(anchor)
    {
      ASSERT(type == TYPE_SYMBOL || type == TYPE_CIRCLE, ());
    }

    m2::PointD m_position = m2::PointD::Zero();
    graphics::EPosition m_anchor = graphics::EPosCenter;
  };

  struct ComplexShape : BaseShape
  {
    ComplexShape(FeatureID geomID, di::DrawRule const & rule, int generation, EShapeType type)
      : BaseShape(rule, generation, type)
      , m_geomID(geomID)
    {
      ASSERT(type == TYPE_AREA ||
             type == TYPE_PATH ||
             type == TYPE_PATH_TEXT ||
             type == TYPE_TEXT ||
             type == TYPE_ROAD_NUMBER, ());

      ASSERT(type == TYPE_ROAD_NUMBER || m_drawRule.m_rule != nullptr, ());
    }

    FeatureID m_geomID;
  };

  struct TextShape : ComplexShape
  {
    TextShape(m2::PointD const & pt, graphics::EPosition anchor, FeatureID const & id,
              di::DrawRule const & rule, int generation, EShapeType type)
      : ComplexShape(id, rule, generation, type)
      , m_position(pt)
      , m_anchor(anchor)
    {
    }

    m2::PointD m_position = m2::PointD::Zero();
    graphics::EPosition m_anchor = graphics::EPosCenter;
  };

  void DrawSymbol(PointShape const * shape);
  void DrawCircle(PointShape const * shape);
  void DrawPath(ComplexShape const * shape);
  void DrawArea(ComplexShape const * shape);
  void DrawPathText(ComplexShape const * shape);
  void DrawRoadNumber(TextShape const * shape);
  void DrawText(TextShape const * shape);

  class CPUOverlayTree;
  struct OverlayWrapper
  {
    OverlayWrapper(BaseShape const ** rules, size_t count)
      : m_ruleCount(count)
    {
      ASSERT(count > 0 && count < 3, ());
      m_rules[0] = rules[0];
      if (m_ruleCount == 2)
        m_rules[1] = rules[1];
    }

    BaseShape const * m_rules[2];
    size_t m_ruleCount;
    vector<m2::RectD> m_rects;
    m2::RectD m_boundRect;
  };

  OverlayWrapper & AddOverlay(BaseShape const * shape1, BaseShape const * shape2 = nullptr);

  using TTextRendererCall = function<void (m2::PointD const &, graphics::EPosition,
                                           graphics::FontDesc const &, graphics::FontDesc const &,
                                           strings::UniString const &, strings::UniString const &)>;
  void CallTextRendererFn(TextShape const * shape, TTextRendererCall const & fn);

  using TRoadNumberRendererCall = function<void (m2::PointD const &, graphics::EPosition,
                                                 graphics::FontDesc const &, strings::UniString const &)>;
  void CallTextRendererFn(TextShape const * shape, TRoadNumberRendererCall const & fn);

  using TPathTextRendererCall = function<void (di::PathInfo const &, graphics::FontDesc const &,
                                               strings::UniString const & text)>;
  void CallTextRendererFn(ComplexShape const * shape, TPathTextRendererCall const & fn);

  list<ComplexShape> m_areaPathShapes;

  // overlay part
  list<PointShape> m_pointShapes;
  list<ComplexShape> m_pathTextShapes;
  list<TextShape> m_textShapes;
  list<OverlayWrapper> m_overlayList;

  map<FeatureID, di::FeatureStyler> m_stylers;
  map<FeatureID, di::AreaInfo> m_areasGeometry;
  map<FeatureID, di::PathInfo> m_pathGeometry;
  map<FeatureID, string> m_roadNames;
  graphics::FontDesc m_roadNumberFont;
};
