#pragma once

#include "drape_frontend/watch/frame_image.hpp"
#include "drape_frontend/watch/feature_processor.hpp"

#include "drape/drape_global.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/screenbase.hpp"

#include "std/function.hpp"
#include "std/list.hpp"
#include "std/unique_ptr.hpp"

namespace df
{
namespace watch
{

class SoftwareRenderer;

class CPUDrawer
{

public:
  struct Params
  {
    Params(string const & resourcesPrefix, double visualScale)
      : m_resourcesPrefix(resourcesPrefix)
      , m_visualScale(visualScale)
    {}

    string m_resourcesPrefix;
    double m_visualScale;
  };

  CPUDrawer(Params const & params);
  ~CPUDrawer();

  void BeginFrame(uint32_t width, uint32_t height, dp::Color const & bgColor);
  void Flush();
  void DrawMyPosition(m2::PointD const & myPxPotision);
  void DrawSearchResult(m2::PointD const & pxPosition);
  void DrawSearchArrow(double azimut);
  void EndFrame(FrameImage & image);

  GlyphCache * GetGlyphCache() const;

  double GetVisualScale() const { return m_visualScale; }

  ScreenBase CalculateScreen(m2::PointD const & center, int zoomModifier,
                             uint32_t pxWidth, uint32_t pxHeight,
                             FrameSymbols const & symbols, int & resultZoom);

  void Draw(FeatureData const & data);

protected:
  void DrawSymbol(m2::PointD const & pt, dp::Anchor pos, DrawRule const & rule);
  void DrawCircle(m2::PointD const & pt, dp::Anchor pos, DrawRule const & rule);
  void DrawCircledSymbol(m2::PointD const & pt, dp::Anchor pos,
                         DrawRule const & symbolRule, DrawRule const & circleRule);
  void DrawPath(PathInfo const & path, DrawRule const * rules, size_t count);
  void DrawArea(AreaInfo const & area, DrawRule const & rule);
  void DrawText(m2::PointD const & pt, dp::Anchor pos,
                FeatureStyler const & fs, DrawRule const & rule);
  void DrawPathText(PathInfo const & info, FeatureStyler const & fs, DrawRule const & rule);
  void DrawPathNumber(PathInfo const & path, FeatureStyler const & fs, DrawRule const & rule);

  using TRoadNumberCallbackFn = function<void (m2::PointD const & pt, dp::FontDecl const & font,
                                               string const & text)>;
  void GenerateRoadNumbers(PathInfo const & path, dp::FontDecl const & font, FeatureStyler const & fs,
                           TRoadNumberCallbackFn const & fn);

  void DrawFeatureStart(FeatureID const & id);
  void DrawFeatureEnd(FeatureID const & id);
  FeatureID const & GetCurrentFeatureID() const;

  int GetGeneration() { return m_generationCounter++; }

  FeatureID const & Insert(PathInfo const & info);
  FeatureID const & Insert(AreaInfo const & info);
  FeatureID const & Insert(FeatureStyler const & styler);
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
    BaseShape(DrawRule const & rule, int generation, EShapeType type)
      : m_drawRule(rule)
      , m_generation(generation)
      , m_type(type)
    {}

    DrawRule m_drawRule;
    int m_generation = 0;
    EShapeType m_type = TYPE_INVALID;
  };

  struct PointShape : BaseShape
  {
    PointShape(m2::PointD const & pt, dp::Anchor anchor, EShapeType type,
               DrawRule const & rule, int generation)
      : BaseShape(rule, generation, type)
      , m_position(pt)
      , m_anchor(anchor)
    {
      ASSERT(type == TYPE_SYMBOL || type == TYPE_CIRCLE, ());
    }

    m2::PointD m_position = m2::PointD::Zero();
    dp::Anchor m_anchor = dp::Center;
  };

  struct ComplexShape : BaseShape
  {
    ComplexShape(FeatureID geomID, DrawRule const & rule, int generation, EShapeType type)
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
    TextShape(m2::PointD const & pt, dp::Anchor anchor, FeatureID const & id,
              DrawRule const & rule, int generation, EShapeType type)
      : ComplexShape(id, rule, generation, type)
      , m_position(pt)
      , m_anchor(anchor)
    {}

    m2::PointD m_position = m2::PointD::Zero();
    dp::Anchor m_anchor = dp::Center;
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

  using TTextRendererCall = function<void (m2::PointD const &, dp::Anchor,
                                           dp::FontDecl const &, dp::FontDecl const &,
                                           strings::UniString const &, strings::UniString const &)>;
  void CallTextRendererFn(TextShape const * shape, TTextRendererCall const & fn);

  using TRoadNumberRendererCall = function<void (m2::PointD const &, dp::Anchor,
                                                 dp::FontDecl const &, strings::UniString const &)>;
  void CallTextRendererFn(TextShape const * shape, TRoadNumberRendererCall const & fn);

  using TPathTextRendererCall = function<void (PathInfo const &, dp::FontDecl const &,
                                               strings::UniString const & text)>;
  void CallTextRendererFn(ComplexShape const * shape, TPathTextRendererCall const & fn);

  list<ComplexShape> m_areaPathShapes;

  // overlay part
  list<PointShape> m_pointShapes;
  list<ComplexShape> m_pathTextShapes;
  list<TextShape> m_textShapes;
  list<OverlayWrapper> m_overlayList;

  map<FeatureID, FeatureStyler> m_stylers;
  map<FeatureID, AreaInfo> m_areasGeometry;
  map<FeatureID, PathInfo> m_pathGeometry;
  map<FeatureID, string> m_roadNames;
  dp::FontDecl m_roadNumberFont;

  double m_visualScale;

  FeatureID m_currentFeatureID;
};

}
}
