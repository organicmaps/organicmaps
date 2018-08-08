#pragma once

#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/drape_diagnostics.hpp"
#include "drape/pointers.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/spline.hpp"

#include <functional>
#include <vector>

class CaptionDefProto;
class ShieldRuleProto;
class SymbolRuleProto;

namespace dp
{
class TextureManager;
} // namespace dp

namespace df
{

struct TextViewParams;
class MapShape;
struct BuildingOutline;

using TInsertShapeFn = std::function<void(drape_ptr<MapShape> && shape)>;

class BaseApplyFeature
{
public:
  BaseApplyFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape,
                   FeatureID const & id, int minVisibleScale, uint8_t rank,
                   CaptionDescription const & captions);

  virtual ~BaseApplyFeature() = default;

  struct HotelData
  {
    bool m_isHotel = false;
    string m_rating;
    int m_stars = 0;
    int m_priceCategory = 0;
  };

  void SetHotelData(HotelData && hotelData);

protected:
  void ExtractCaptionParams(CaptionDefProto const * primaryProto,
                            CaptionDefProto const * secondaryProto,
                            float depth, TextViewParams & params) const;
  string ExtractHotelInfo() const;

  TInsertShapeFn m_insertShape;
  FeatureID m_id;
  CaptionDescription const & m_captions;
  int m_minVisibleScale;
  uint8_t m_rank;
  HotelData m_hotelData;

  TileKey const m_tileKey;
  m2::RectD const m_tileRect;
};

class ApplyPointFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyPointFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape,
                    FeatureID const & id, int minVisibleScale, uint8_t rank,
                    CaptionDescription const & captions, float posZ,
                    int displacementMode, DepthLayer depthLayer);

  void operator()(m2::PointD const & point, bool hasArea, bool isUGC);
  void ProcessPointRule(Stylist::TRuleWrapper const & rule);
  void Finish(ref_ptr<dp::TextureManager> texMng);

protected:
  float const m_posZ;

private:
  bool m_hasPoint;
  bool m_hasArea;
  bool m_createdByEditor;
  bool m_obsoleteInEditor;
  bool m_isUGC;
  DepthLayer m_depthLayer;
  double m_symbolDepth;
  SymbolRuleProto const * m_symbolRule;
  m2::PointF m_centerPoint;
  int m_displacementMode;
  std::vector<TextViewParams> m_textParams;
};

class ApplyAreaFeature : public ApplyPointFeature
{
  using TBase = ApplyPointFeature;

public:
  ApplyAreaFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape,
                   FeatureID const & id, double currentScaleGtoP, bool isBuilding,
                   bool skipAreaGeometry, float minPosZ, float posZ, int minVisibleScale,
                   uint8_t rank, CaptionDescription const & captions, bool hatchingArea);

  using TBase::operator ();

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void ProcessAreaRule(Stylist::TRuleWrapper const & rule);

private:
  using TEdge = pair<int, int>;

  void ProcessBuildingPolygon(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void CalculateBuildingOutline(bool calculateNormals, BuildingOutline & outline);
  int GetIndex(m2::PointD const & pt);
  void BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3);
  bool EqualEdges(TEdge const & edge1, TEdge const & edge2) const;
  bool FindEdge(TEdge const & edge);
  m2::PointD CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const;

  std::vector<m2::PointD> m_triangles;

  buffer_vector<m2::PointD, kBuildingOutlineSize> m_points;
  buffer_vector<pair<TEdge, int>, kBuildingOutlineSize> m_edges;

  float const m_minPosZ;
  bool const m_isBuilding;
  bool const m_skipAreaGeometry;
  bool const m_hatchingArea;
  double const m_currentScaleGtoP;
};

class ApplyLineFeatureGeometry : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureGeometry(TileKey const & tileKey, TInsertShapeFn const & insertShape,
                           FeatureID const & id, double currentScaleGtoP, int minVisibleScale,
                           uint8_t rank, size_t pointsCount);

  void operator() (m2::PointD const & point);
  bool HasGeometry() const;
  void ProcessLineRule(Stylist::TRuleWrapper const & rule);
  void Finish();

  std::vector<m2::SharedSpline> const & GetClippedSplines() const { return m_clippedSplines; }

private:
  m2::SharedSpline m_spline;
  std::vector<m2::SharedSpline> m_clippedSplines;
  float m_currentScaleGtoP;
  double m_sqrScale;
  m2::PointD m_lastAddedPoint;
  bool m_simplify;
  size_t m_initialPointsCount;

#ifdef LINES_GENERATION_CALC_FILTERED_POINTS
  int m_readCount;
#endif
};

class ApplyLineFeatureAdditional : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeatureAdditional(TileKey const & tileKey, TInsertShapeFn const & insertShape,
                             FeatureID const & id, double currentScaleGtoP, int minVisibleScale,
                             uint8_t rank, CaptionDescription const & captions,
                             std::vector<m2::SharedSpline> const & clippedSplines);

  void ProcessLineRule(Stylist::TRuleWrapper const & rule);
  void Finish(ref_ptr<dp::TextureManager> texMng, std::set<ftypes::RoadShield> && roadShields,
              GeneratedRoadShields & generatedRoadShields);

private:
  void GetRoadShieldsViewParams(ref_ptr<dp::TextureManager> texMng,
                                ftypes::RoadShield const & shield,
                                uint8_t shieldIndex, uint8_t shieldCount,
                                TextViewParams & textParams,
                                ColoredSymbolViewParams & symbolParams,
                                PoiSymbolViewParams & poiParams,
                                m2::PointD & shieldPixelSize);
  bool CheckShieldsNearby(m2::PointD const & shieldPos,
                          m2::PointD const & shieldPixelSize,
                          uint32_t minDistanceInPixels,
                          std::vector<m2::RectD> & shields);

  std::vector<m2::SharedSpline> m_clippedSplines;
  float m_currentScaleGtoP;
  float m_depth;
  CaptionDefProto const * m_captionRule;
  ShieldRuleProto const * m_shieldRule;
};

extern dp::Color ToDrapeColor(uint32_t src);
} // namespace df
