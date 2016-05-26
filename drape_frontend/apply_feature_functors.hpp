#pragma once

#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"

#include "indexer/point_to_int64.hpp"

#include "geometry/point2d.hpp"
#include "geometry/spline.hpp"

#include "std/unordered_map.hpp"

class CaptionDefProto;
class CircleRuleProto;
class ShieldRuleProto;
class SymbolRuleProto;

//#define CALC_FILTERED_POINTS

namespace df
{

struct TextViewParams;
class MapShape;
struct BuildingEdge;

using TInsertShapeFn = function<void(drape_ptr<MapShape> && shape)>;

class BaseApplyFeature
{
public:
  BaseApplyFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
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
};

class ApplyPointFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyPointFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
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
  double m_circleDepth;
  SymbolRuleProto const * m_symbolRule;
  CircleRuleProto const * m_circleRule;
  m2::PointF m_centerPoint;
};

class ApplyAreaFeature : public ApplyPointFeature
{
  using TBase = ApplyPointFeature;

public:
  ApplyAreaFeature(TInsertShapeFn const & insertShape, FeatureID const & id, m2::RectD tileRect, float minPosZ,
                   float posZ, int minVisibleScale, uint8_t rank, CaptionDescription const & captions);

  using TBase::operator ();

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void ProcessRule(Stylist::TRuleWrapper const & rule);

private:
  using TEdge = pair<int, int>;

  void ProcessBuildingPolygon(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void CalculateBuildingEdges(vector<BuildingEdge> & edges);
  int GetIndex(m2::PointD const & pt);
  void BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3);
  bool EqualEdges(TEdge const & edge1, TEdge const & edge2) const;
  bool FindEdge(TEdge const & edge);
  m2::PointD CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const;

  vector<m2::PointF> m_triangles;

  unordered_map<int, m2::PointD> m_indices;
  vector<pair<TEdge, int>> m_edges;
  float const m_minPosZ;
  bool const m_isBuilding;
  m2::RectD m_tileRect;
};

class ApplyLineFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeature(TInsertShapeFn const & insertShape, FeatureID const & id, m2::RectD tileRect,
                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                   double currentScaleGtoP, bool simplify, size_t pointsCount);

  void operator() (m2::PointD const & point);
  bool HasGeometry() const;
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish();

private:
  m2::SharedSpline m_spline;
  vector<m2::SharedSpline> m_clippedSplines;
  double m_currentScaleGtoP;
  double m_sqrScale;
  m2::PointD m_lastAddedPoint;
  bool m_simplify;
  size_t m_initialPointsCount;
  double m_shieldDepth;
  ShieldRuleProto const * m_shieldRule;
  m2::RectD m_tileRect;

#ifdef CALC_FILTERED_POINTS
  int m_readedCount;
#endif
};

} // namespace df
