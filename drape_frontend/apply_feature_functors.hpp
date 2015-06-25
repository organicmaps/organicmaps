#pragma once

#include "drape_frontend/stylist.hpp"
#include "drape_frontend/tile_key.hpp"

#include "drape/pointers.hpp"

#include "indexer/point_to_int64.hpp"

#include "geometry/point2d.hpp"
#include "geometry/spline.hpp"

class CircleRuleProto;
class SymbolRuleProto;
class CaptionDefProto;

//#define CALC_FILTERED_POINTS

namespace df
{

struct TextViewParams;
class EngineContext;

class BaseApplyFeature
{
public:
  BaseApplyFeature(ref_ptr<EngineContext> context, FeatureID const & id,
                   CaptionDescription const & captions);

  virtual ~BaseApplyFeature() {}

protected:
  void ExtractCaptionParams(CaptionDefProto const * primaryProto,
                            CaptionDefProto const * secondaryProto,
                            double depth,
                            TextViewParams & params) const;

  ref_ptr<EngineContext> m_context;
  FeatureID m_id;
  CaptionDescription const & m_captions;
};

class ApplyPointFeature : public BaseApplyFeature
{
  typedef BaseApplyFeature TBase;
public:
  ApplyPointFeature(ref_ptr<EngineContext> context, FeatureID const & id,
                    CaptionDescription const & captions);

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
  typedef ApplyPointFeature TBase;
public:
  ApplyAreaFeature(ref_ptr<EngineContext> context, FeatureID const & id,
                   CaptionDescription const & captions);

  using TBase::operator ();

  void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
  void ProcessRule(Stylist::TRuleWrapper const & rule);

private:
  vector<m2::PointF> m_triangles;
};

class ApplyLineFeature : public BaseApplyFeature
{
  typedef BaseApplyFeature TBase;
public:
  ApplyLineFeature(ref_ptr<EngineContext> context, FeatureID const & id,
                   CaptionDescription const & captions,
                   double currentScaleGtoP);

  void operator() (m2::PointD const & point);
  bool HasGeometry() const;
  void ProcessRule(Stylist::TRuleWrapper const & rule);
  void Finish();

private:
  m2::SharedSpline m_spline;
  double m_currentScaleGtoP;
  double m_sqrScale;
  m2::PointD m_lastAddedPoint;

#ifdef CALC_FILTERED_POINTS
  int m_readedCount;
#endif
};

} // namespace df
