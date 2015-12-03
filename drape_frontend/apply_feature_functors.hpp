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

protected:
  void ExtractCaptionParams(CaptionDefProto const * primaryProto,
                            CaptionDefProto const * secondaryProto,
                            double depth, TextViewParams & params) const;

  TInsertShapeFn m_insertShape;
  FeatureID m_id;
  CaptionDescription const & m_captions;
  int m_minVisibleScale;
  uint8_t m_rank;
};

class ApplyPointFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyPointFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
                    int minVisibleScale, uint8_t rank, CaptionDescription const & captions);

  void operator()(m2::PointD const & point);
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish();

private:
  bool m_hasPoint;
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
  ApplyAreaFeature(TInsertShapeFn const & insertShape, FeatureID const & id, bool isBuilding,
                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions);

  using TBase::operator ();

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void ProcessRule(Stylist::TRuleWrapper const & rule);

private:
  using TEdge = pair<int, int>;

  void CalculateBuildingEdges(vector<BuildingEdge> & edges);
  int GetIndex(m2::PointD const & pt);
  void BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3);
  bool EqualEdges(TEdge const & edge1, TEdge const & edge2) const;
  bool FindEdge(TEdge const & edge) const;
  m2::PointD CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const;

  vector<m2::PointF> m_triangles;

  unordered_map<int, m2::PointD> m_indices;
  vector<pair<TEdge, int>> m_edges;
  bool const m_isBuilding;
};

class ApplyLineFeature : public BaseApplyFeature
{
  using TBase = BaseApplyFeature;

public:
  ApplyLineFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                   double currentScaleGtoP, bool simplify, size_t pointsCount);

  void operator() (m2::PointD const & point);
  bool HasGeometry() const;
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish();

private:
  m2::SharedSpline m_spline;
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

} // namespace df
