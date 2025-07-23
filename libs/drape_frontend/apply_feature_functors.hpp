#pragma once

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/road_shields_parser.hpp"

#include "geometry/point2d.hpp"
#include "geometry/spline.hpp"

#include <functional>
#include <vector>

class CaptionDefProto;

namespace dp
{
class TextureManager;
}  // namespace dp

namespace df
{

struct TextViewParams;
class MapShape;
struct BuildingOutline;

using TInsertShapeFn = std::function<void(drape_ptr<MapShape> && shape)>;

class BaseApplyFeature
{
public:
  BaseApplyFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureType & f,
                   CaptionDescription const & captions);

  virtual ~BaseApplyFeature() = default;

protected:
  void FillCommonParams(CommonOverlayViewParams & p) const;
  double PriorityToDepth(int priority, drule::TypeT ruleType, double areaDepth) const;

  TInsertShapeFn m_insertShape;
  FeatureType & m_f;
  CaptionDescription const & m_captions;

  TileKey const m_tileKey;
  m2::RectD const m_tileRect;
};

class ApplyPointFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyPointFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureType & f,
                    CaptionDescription const & captions);

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
  ApplyAreaFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureType & f,
                   double currentScaleGtoP, bool isBuilding, float minPosZ, float posZ,
                   CaptionDescription const & captions);

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  bool HasGeometry() const { return !m_triangles.empty(); }
  void ProcessAreaRules(AreaRuleProto const * areaRule, AreaRuleProto const * hatchingRule);

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

  void ProcessRule(AreaRuleProto const & areaRule, double areaDepth, bool isHatching);
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
  double const m_currentScaleGtoP;
};

class ApplyLineFeatureGeometry : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureGeometry(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureType & f,
                           double currentScaleGtoP);

  void operator()(m2::PointD const & point);
  bool HasGeometry() const { return m_spline->IsValid(); }
  void ProcessLineRules(Stylist::LineRulesT const & lineRules);

  std::vector<m2::SharedSpline> const & GetClippedSplines() const { return m_clippedSplines; }

private:
  void ProcessRule(LineRuleProto const & lineRule);

  m2::SharedSpline m_spline;
  std::vector<m2::SharedSpline> m_clippedSplines;
  double const m_currentScaleGtoP;
  double const m_minSegmentSqrLength;
  m2::PointD m_lastAddedPoint;
  bool const m_simplify;

#ifdef LINES_GENERATION_CALC_FILTERED_POINTS
  int m_readCount = 0;
#endif
};

// Process pathtext and shield drules. Operates on metalines usually.
class ApplyLineFeatureAdditional : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureAdditional(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureType & f,
                             double currentScaleGtoP, CaptionDescription const & captions,
                             std::vector<m2::SharedSpline> && clippedSplines);

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
  double const m_currentScaleGtoP;
  float m_captionDepth = 0.0f, m_shieldDepth = 0.0f;
  CaptionDefProto const * m_captionRule = nullptr;
  ShieldRuleProto const * m_shieldRule = nullptr;
};

extern dp::Color ToDrapeColor(uint32_t src);
}  // namespace df
