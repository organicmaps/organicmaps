#pragma once

#include "drape_frontend/relations_draw_info.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/stylist.hpp"

#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/road_shields_parser.hpp"

#include "geometry/point2d.hpp"
#include "geometry/spline.hpp"

#include <vector>

class CaptionDefProto;

namespace dp
{
class TextureManager;
}  // namespace dp

namespace df
{

struct TextViewParams;
struct BuildingOutline;

struct ApplyFeatureParams;
class BaseApplyFeature
{
public:
  using Params = ApplyFeatureParams;
  BaseApplyFeature(Params const & params, FeatureType & f, CaptionDescription const & captions)
    : m_params(params)
    , m_f(f)
    , m_captions(captions)
  {}

protected:
  void FillCommonParams(CommonOverlayViewParams & p) const;
  double PriorityToDepth(int priority, drule::TypeT ruleType, double areaDepth) const;

  Params const & m_params;
  FeatureType & m_f;
  CaptionDescription const & m_captions;
};

class ApplyPointFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyPointFeature(Params const & params, FeatureType & f, CaptionDescription const & captions)
    : TBase(params, f, captions)
  {}

  void ProcessPointRules(SymbolRuleProto const * symbolRule, CaptionRuleProto const * captionRule,
                         CaptionRuleProto const * houseNumberRule, m2::PointD const & centerPoint,
                         ref_ptr<dp::TextureManager> texMng);

protected:
  void ExtractCaptionParams(CaptionDefProto const * primaryProto, CaptionDefProto const * secondaryProto,
                            TextViewParams & params) const;
  float m_posZ = 0.0f;

private:
  virtual bool HasArea() const { return false; }
};

class ApplyAreaFeature : public ApplyPointFeature
{
  using TBase = ApplyPointFeature;

public:
  ApplyAreaFeature(Params const & params, FeatureType & f, CaptionDescription const & captions, bool isBuilding,
                   bool isMwmBorder, float minPosZ, float posZ)
    : TBase(params, f, captions)
    , m_minPosZ(minPosZ)
    , m_isBuilding(isBuilding)
    , m_isMwmBorder(isMwmBorder)
  {
    m_posZ = posZ;
  }

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  bool HasGeometry() const { return !m_triangles.empty(); }
  void ProcessAreaRules(AreaRuleProto const * areaRule, AreaRuleProto const * hatchingRule, std::string_view hatchKey);

  struct Edge
  {
    Edge() = default;
    Edge(int startIndex, int endIndex) : m_startIndex(startIndex), m_endIndex(endIndex) {}

    bool operator==(Edge const & edge) const
    {
      return (m_startIndex == edge.m_startIndex && m_endIndex == edge.m_endIndex) ||
             (m_startIndex == edge.m_endIndex && m_endIndex == edge.m_startIndex);
    }

    int m_startIndex = -1;
    int m_endIndex = -1;
  };

  struct ExtendedEdge
  {
    ExtendedEdge() = default;
    ExtendedEdge(Edge && edge, int internalVertexIndex, bool twoSide)
      : m_edge(std::move(edge))
      , m_internalVertexIndex(internalVertexIndex)
      , m_twoSide(twoSide)
    {}

    Edge m_edge;
    int m_internalVertexIndex = -1;
    bool m_twoSide = false;
  };

private:
  bool HasArea() const override { return true; }

  void ProcessRule(AreaRuleProto const & areaRule, double areaDepth, std::string_view hatchKey);
  void ProcessBuildingPolygon(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void CalculateBuildingOutline(bool calculateNormals, BuildingOutline & outline);
  int GetIndex(m2::PointD const & pt);
  void BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3, bool twoSide);
  bool IsDuplicatedEdge(Edge const & edge);
  m2::PointD CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const;

  std::vector<m2::PointD> m_triangles;
  buffer_vector<m2::PointD, kBuildingOutlineSize> m_points;
  buffer_vector<ExtendedEdge, kBuildingOutlineSize> m_edges;

  float const m_minPosZ;
  bool const m_isBuilding;
  bool const m_isMwmBorder;
};

class ApplyLineFeatureGeometry : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureGeometry(Params const & params, FeatureType & f, RelationsDrawSettings const & relsSettings);

  void operator()(m2::PointD const & point);
  bool HasGeometry() const { return m_spline->IsValid(); }
  void ProcessLineRules(Stylist::LineRulesT const & lineRules, bool isIsoline);

  std::vector<m2::SharedSpline> MoveClippedSplines() const { return std::move(m_clippedSplines); }

private:
  void ProcessRule(LineRuleProto const & lineRule);

  RelationsDrawInfo m_relsInfo;
  m2::SharedSpline m_spline;
  std::vector<m2::SharedSpline> m_clippedSplines;
  m2::PointD m_lastAddedPoint;

#ifdef LINES_GENERATION_CALC_FILTERED_POINTS
  int m_readCount = 0;
#endif
};

// Process pathtext and shield drules. Operates on metalines usually.
class ApplyLineFeatureAdditional : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureAdditional(Params const & params, FeatureType & f, CaptionDescription const & captions,
                             std::vector<m2::SharedSpline> && clippedSplines)
    : TBase(params, f, captions)
    , m_clippedSplines(std::move(clippedSplines))
  {
    ASSERT(!m_clippedSplines.empty(), ());
  }

  void ProcessAdditionalLineRules(PathTextRuleProto const * pathtextRule, ShieldRuleProto const * shieldRule,
                                  ref_ptr<dp::TextureManager> texMng, ftypes::RoadShieldsSetT const & roadShields,
                                  GeneratedRoadShields & generatedRoadShields);

private:
  void GetRoadShieldsViewParams(ref_ptr<dp::TextureManager> texMng, ftypes::RoadShield const & shield,
                                uint8_t shieldIndex, uint8_t shieldCount, TextViewParams & textParams,
                                ColoredSymbolViewParams & symbolParams, PoiSymbolViewParams & poiParams,
                                m2::PointD & shieldPixelSize);
  bool CheckShieldsNearby(m2::PointD const & shieldPos, m2::PointD const & shieldPixelSize,
                          uint32_t minDistanceInPixels, std::vector<m2::RectD> & shields);

  std::vector<m2::SharedSpline> m_clippedSplines;
  float m_captionDepth = 0.0f, m_shieldDepth = 0.0f;
  CaptionDefProto const * m_captionRule = nullptr;
  ShieldRuleProto const * m_shieldRule = nullptr;
};

extern dp::Color ToDrapeColor(uint32_t src);
}  // namespace df
