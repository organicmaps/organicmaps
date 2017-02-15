#pragma once

#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/pointers.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/point_to_int64.hpp"

#include "geometry/point2d.hpp"
#include "geometry/polyline2d.hpp"
#include "geometry/spline.hpp"

#include "std/unordered_map.hpp"

class CaptionDefProto;
class ShieldRuleProto;
class SymbolRuleProto;

//#define CALC_FILTERED_POINTS

namespace ftypes
{
struct RoadShield;
}

namespace df
{

struct TextViewParams;
class MapShape;
struct BuildingOutline;

using TInsertShapeFn = function<void(drape_ptr<MapShape> && shape)>;

class BaseApplyFeature
{
public:
  BaseApplyFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions);

  virtual ~BaseApplyFeature() {}

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
                            double depth, TextViewParams & params) const;
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
  ApplyPointFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                    int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                    float posZ);

  void operator()(m2::PointD const & point, bool hasArea);
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish();

protected:
  float const m_posZ;

private:
  bool m_hasPoint;
  bool m_hasArea;
  bool m_createdByEditor;
  bool m_obsoleteInEditor;
  double m_symbolDepth;
  SymbolRuleProto const * m_symbolRule;
  m2::PointF m_centerPoint;
};

class ApplyAreaFeature : public ApplyPointFeature
{
  using TBase = ApplyPointFeature;

public:
  ApplyAreaFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                   bool isBuilding, float minPosZ, float posZ, int minVisibleScale,
                   uint8_t rank, CaptionDescription const & captions);

  using TBase::operator ();

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void ProcessRule(Stylist::TRuleWrapper const & rule);

private:
  using TEdge = pair<int, int>;

  void ProcessBuildingPolygon(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void CalculateBuildingOutline(bool calculateNormals, BuildingOutline & outline);
  int GetIndex(m2::PointD const & pt);
  void BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3);
  bool EqualEdges(TEdge const & edge1, TEdge const & edge2) const;
  bool FindEdge(TEdge const & edge);
  m2::PointD CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const;

  vector<m2::PointD> m_triangles;

  buffer_vector<m2::PointD, kBuildingOutlineSize> m_points;
  buffer_vector<pair<TEdge, int>, kBuildingOutlineSize> m_edges;

  float const m_minPosZ;
  bool const m_isBuilding;
};

class ApplyLineFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                   double currentScaleGtoP, int minVisibleScale, uint8_t rank,
                   CaptionDescription const & captions, size_t pointsCount);

  void operator() (m2::PointD const & point);
  bool HasGeometry() const;
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish(std::vector<ftypes::RoadShield> && roadShields);

  m2::PolylineD GetPolyline() const;

private:
  void GetRoadShieldsViewParams(ftypes::RoadShield const & shield,
                                TextViewParams & textParams,
                                ColoredSymbolViewParams & symbolParams,
                                PoiSymbolViewParams & poiParams);

  m2::SharedSpline m_spline;
  vector<m2::SharedSpline> m_clippedSplines;
  double m_currentScaleGtoP;
  double m_sqrScale;
  m2::PointD m_lastAddedPoint;
  bool m_simplify;
  size_t m_initialPointsCount;
  double m_shieldDepth;
  ShieldRuleProto const * m_shieldRule;

#ifdef CALC_FILTERED_POINTS
  int m_readedCount;
#endif
};

extern dp::Color ToDrapeColor(uint32_t src);

} // namespace df
