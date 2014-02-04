#pragma once

#include "stylist.hpp"
#include "tile_key.hpp"

#include "../indexer/point_to_int64.hpp"

#include "../geometry/point2d.hpp"


class CircleRuleProto;
class SymbolRuleProto;

namespace df
{
  class EngineContext;

  class BaseApplyFeature
  {
  public:
    BaseApplyFeature(EngineContext & context,
                     TileKey tileKey);

  protected:
    EngineContext & m_context;
    TileKey m_tileKey;
  };

  class ApplyPointFeature : public BaseApplyFeature
  {
    typedef BaseApplyFeature base_t;
  public:
    ApplyPointFeature(EngineContext & context, TileKey tileKey);

    void operator()(CoordPointT const & point);
    void operator()(m2::PointD const & point);
    void ProcessRule(Stylist::rule_wrapper_t const & rule);
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
    typedef ApplyPointFeature base_t;
  public:
    ApplyAreaFeature(EngineContext & context, TileKey tileKey);

    using base_t::operator ();

    void operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3);
    void ProcessRule(Stylist::rule_wrapper_t const & rule);

  private:
    vector<m2::PointF> m_triangles;
  };

  class ApplyLineFeature : public BaseApplyFeature
  {
    typedef BaseApplyFeature base_t;
  public:
    ApplyLineFeature(EngineContext & context, TileKey tileKey);

    void operator ()(CoordPointT const & point);
    void ProcessRule(Stylist::rule_wrapper_t const & rule);

  private:
    vector<m2::PointF> m_path;
  };
}
