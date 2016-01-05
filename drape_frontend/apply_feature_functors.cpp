#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/circle_shape.hpp"
#include "drape_frontend/path_text_shape.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"

#include "drape/color.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/utils/projection.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/utility.hpp"
#include "std/mutex.hpp"

namespace df
{

namespace
{

double const kMinVisibleFontSize = 8.0;

dp::Color ToDrapeColor(uint32_t src)
{
  return dp::Extract(src, 255 - (src >> 24));
}

#ifdef CALC_FILTERED_POINTS
class LinesStat
{
public:
  ~LinesStat()
  {
    map<int, TValue> zoomValues;
    for (pair<TKey, TValue> const & f : m_features)
    {
      TValue & v = zoomValues[f.first.second];
      v.m_neededPoints += f.second.m_neededPoints;
      v.m_readedPoints += f.second.m_readedPoints;
    }

    LOG(LINFO, ("========================"));
    for (pair<int, TValue> const & v : zoomValues)
      LOG(LINFO, ("Zoom = ", v.first, " Percent = ", 1 - v.second.m_neededPoints / (double)v.second.m_readedPoints));
  }

  static LinesStat & Get()
  {
    static LinesStat s_stat;
    return s_stat;
  }

  void InsertLine(FeatureID const & id, double scale, int vertexCount, int renderVertexCount)
  {
    int s = 0;
    double factor = 5.688;
    while (factor < scale)
    {
      s++;
      factor = factor * 2.0;
    }

    InsertLine(id, s, vertexCount, renderVertexCount);
  }

  void InsertLine(FeatureID const & id, int scale, int vertexCount, int renderVertexCount)
  {
    TKey key(id, scale);
    lock_guard<mutex> g(m_mutex);
    if (m_features.find(key) != m_features.end())
      return;

    TValue & v = m_features[key];
    v.m_readedPoints = vertexCount;
    v.m_neededPoints = renderVertexCount;
  }

private:
  LinesStat() = default;

  using TKey = pair<FeatureID, int>;
  struct TValue
  {
    int m_readedPoints = 0;
    int m_neededPoints = 0;
  };

  map<TKey, TValue> m_features;
  mutex m_mutex;
};
#endif

void Extract(::LineDefProto const * lineRule,
             df::LineViewParams & params)
{
  float const scale = df::VisualParams::Instance().GetVisualScale();
  params.m_color = ToDrapeColor(lineRule->color());
  params.m_width = max(lineRule->width() * scale, 1.0);

  if (lineRule->has_dashdot())
  {
    DashDotProto const & dd = lineRule->dashdot();

    int const count = dd.dd_size();
    params.m_pattern.reserve(count);
    for (int i = 0; i < count; ++i)
      params.m_pattern.push_back(dd.dd(i) * scale);
  }

  switch(lineRule->cap())
  {
  case ::ROUNDCAP : params.m_cap = dp::RoundCap;
    break;
  case ::BUTTCAP  : params.m_cap = dp::ButtCap;
    break;
  case ::SQUARECAP: params.m_cap = dp::SquareCap;
    break;
  default:
    ASSERT(false, ());
  }

  switch (lineRule->join())
  {
  case ::NOJOIN    : params.m_join = dp::MiterJoin;
    break;
  case ::ROUNDJOIN : params.m_join = dp::RoundJoin;
    break;
  case ::BEVELJOIN : params.m_join = dp::BevelJoin;
    break;
  default:
    ASSERT(false, ());
  }
}

void CaptionDefProtoToFontDecl(CaptionDefProto const * capRule, dp::FontDecl &params)
{
  params.m_color = ToDrapeColor(capRule->color());
  params.m_size = max(kMinVisibleFontSize, capRule->height() * df::VisualParams::Instance().GetVisualScale());

  if (capRule->has_stroke_color())
    params.m_outlineColor = ToDrapeColor(capRule->stroke_color());
}

void ShieldRuleProtoToFontDecl(ShieldRuleProto const * shieldRule, dp::FontDecl &params)
{
  params.m_color = ToDrapeColor(shieldRule->color());
  params.m_size = max(kMinVisibleFontSize, shieldRule->height() * df::VisualParams::Instance().GetVisualScale());

  if (shieldRule->has_stroke_color())
    params.m_outlineColor = ToDrapeColor(shieldRule->stroke_color());
}

dp::Anchor GetAnchor(CaptionDefProto const * capRule)
{
  if (capRule->has_offset_y())
  {
    if (capRule->offset_y() > 0)
      return dp::Top;
    else
      return dp::Bottom;
  }
  if (capRule->has_offset_x())
  {
    if (capRule->offset_x() > 0)
      return dp::Right;
    else
      return dp::Left;

  }

  return dp::Center;
}

m2::PointF GetOffset(CaptionDefProto const * capRule)
{
  float vs = VisualParams::Instance().GetVisualScale();
  m2::PointF result(0, 0);
  if (capRule != nullptr && capRule->has_offset_x())
    result.x = capRule->offset_x() * vs;
  if (capRule != nullptr && capRule->has_offset_y())
    result.y = capRule->offset_y() * vs;

  return result;
}

} // namespace

BaseApplyFeature::BaseApplyFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
                                   int minVisibleScale, uint8_t rank, CaptionDescription const & caption)
  : m_insertShape(insertShape)
  , m_id(id)
  , m_captions(caption)
  , m_minVisibleScale(minVisibleScale)
  , m_rank(rank)
{
  ASSERT(m_insertShape != nullptr, ());
}

void BaseApplyFeature::ExtractCaptionParams(CaptionDefProto const * primaryProto,
                                            CaptionDefProto const * secondaryProto,
                                            double depth, TextViewParams & params) const
{
  dp::FontDecl decl;
  CaptionDefProtoToFontDecl(primaryProto, decl);

  params.m_anchor = GetAnchor(primaryProto);
  params.m_depth = depth;
  params.m_featureID = m_id;
  params.m_primaryText = m_captions.GetMainText();
  params.m_primaryTextFont = decl;
  params.m_primaryOffset = GetOffset(primaryProto);
  params.m_primaryOptional = primaryProto->has_is_optional() ? primaryProto->is_optional() : true;
  params.m_secondaryOptional = true;

  if (secondaryProto)
  {
    dp::FontDecl auxDecl;
    CaptionDefProtoToFontDecl(secondaryProto, auxDecl);

    params.m_secondaryText = m_captions.GetAuxText();
    params.m_secondaryTextFont = auxDecl;
    params.m_secondaryOptional = secondaryProto->has_is_optional() ? secondaryProto->is_optional() : true;
  }
}

ApplyPointFeature::ApplyPointFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
                                     int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                                     float posZ)
  : TBase(insertShape, id, minVisibleScale, rank, captions)
  , m_posZ(posZ)
  , m_hasPoint(false)
  , m_symbolDepth(dp::minDepth)
  , m_circleDepth(dp::minDepth)
  , m_symbolRule(NULL)
  , m_circleRule(NULL)
{
}

void ApplyPointFeature::operator()(m2::PointD const & point)
{
  m_hasPoint = true;
  m_centerPoint = point;
}

void ApplyPointFeature::ProcessRule(Stylist::TRuleWrapper const & rule)
{
  if (m_hasPoint == false)
    return;

  drule::BaseRule const * pRule = rule.first;
  float const depth = rule.second;

  SymbolRuleProto const * symRule = pRule->GetSymbol();
  if (symRule != nullptr)
  {
    m_symbolDepth = depth;
    m_symbolRule = symRule;
  }

  CircleRuleProto const * circleRule = pRule->GetCircle();
  if (circleRule != nullptr)
  {
    m_circleDepth = depth;
    m_circleRule = circleRule;
  }

  bool const hasPOI = (m_symbolRule != nullptr || m_circleRule != nullptr);
  bool const isNode = (pRule->GetType() & drule::node) != 0;
  CaptionDefProto const * capRule = pRule->GetCaption(0);
  if (capRule && isNode)
  {
    TextViewParams params;
    ExtractCaptionParams(capRule, pRule->GetCaption(1), depth, params);
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_posZ = m_posZ;
    if(!params.m_primaryText.empty() || !params.m_secondaryText.empty())
      m_insertShape(make_unique_dp<TextShape>(m_centerPoint, params, hasPOI));
  }
}

void ApplyPointFeature::Finish()
{
  if (m_circleRule && m_symbolRule)
  {
    // draw circledSymbol
  }
  else if (m_circleRule)
  {
    CircleViewParams params(m_id);
    params.m_depth = m_circleDepth;
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_color = ToDrapeColor(m_circleRule->color());
    params.m_radius = m_circleRule->radius();
    m_insertShape(make_unique_dp<CircleShape>(m_centerPoint, params));
  }
  else if (m_symbolRule)
  {
    PoiSymbolViewParams params(m_id);
    params.m_depth = m_symbolDepth;
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_symbolName = m_symbolRule->name();
    float const mainScale = df::VisualParams::Instance().GetVisualScale();
    params.m_extendingSize = m_symbolRule->has_min_distance() ? mainScale * m_symbolRule->min_distance() : 0;
    params.m_posZ = m_posZ;
    m_insertShape(make_unique_dp<PoiSymbolShape>(m_centerPoint, params));
  }
}

ApplyAreaFeature::ApplyAreaFeature(TInsertShapeFn const & insertShape, FeatureID const & id, float minPosZ, float posZ,
                                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions)
  : TBase(insertShape, id, minVisibleScale, rank, captions, posZ)
  , m_minPosZ(minPosZ)
  , m_isBuilding(posZ > 0.0f)
{}

void ApplyAreaFeature::operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
{
  if (m_isBuilding)
  {
    ProcessBuildingPolygon(p1, p2, p3);
    return;
  }

  m_triangles.push_back(p1);
  if (m2::CrossProduct(p2 - p1, p3 - p1) < 0)
  {
    m_triangles.push_back(p2);
    m_triangles.push_back(p3);
  }
  else
  {
    m_triangles.push_back(p3);
    m_triangles.push_back(p2);
  }
}

void ApplyAreaFeature::ProcessBuildingPolygon(m2::PointD const & p1, m2::PointD const & p2,
                                              m2::PointD const & p3)
{
  // For building we must filter degenerate polygons because now we have to reconstruct
  // building outline by bunch of polygons.
  m2::PointD const v1 = p2 - p1;
  m2::PointD const v2 = p3 - p1;
  if (v1.IsAlmostZero() || v2.IsAlmostZero())
    return;

  double const crossProduct = m2::CrossProduct(v1.Normalize(), v2.Normalize());
  double const kEps = 0.01;
  if (fabs(crossProduct) < kEps)
    return;

  m_triangles.push_back(p1);

  if (crossProduct < 0)
  {
    m_triangles.push_back(p2);
    m_triangles.push_back(p3);
    BuildEdges(GetIndex(p1), GetIndex(p2), GetIndex(p3));
  }
  else
  {
    m_triangles.push_back(p3);
    m_triangles.push_back(p2);
    BuildEdges(GetIndex(p1), GetIndex(p3), GetIndex(p2));
  }
}

int ApplyAreaFeature::GetIndex(m2::PointD const & pt)
{
  int maxIndex = -1;
  for (auto it = m_indices.begin(); it != m_indices.end(); ++it)
  {
    if (it->first > maxIndex)
      maxIndex = it->first;

    if (pt.EqualDxDy(it->second, 1e-7))
      return it->first;
  }

  int const newIndex = maxIndex + 1;
  m_indices.insert(make_pair(newIndex, pt));
  return newIndex;
}

bool ApplyAreaFeature::EqualEdges(TEdge const & edge1, TEdge const & edge2) const
{
  return (edge1.first == edge2.first && edge1.second == edge2.second) ||
         (edge1.first == edge2.second && edge1.second == edge2.first);
}

bool ApplyAreaFeature::FindEdge(TEdge const & edge)
{
  for (size_t i = 0; i < m_edges.size(); i++)
  {
    if (EqualEdges(m_edges[i].first, edge))
    {
      m_edges[i].second = -1;
      return true;
    }
  }
  return false;
}

m2::PointD ApplyAreaFeature::CalculateNormal(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3) const
{
  m2::PointD const tangent = (p2 - p1).Normalize();
  m2::PointD normal = m2::PointD(-tangent.y, tangent.x);
  m2::PointD const v = ((p1 + p2) * 0.5 - p3).Normalize();
  if (m2::DotProduct(normal, v) < 0.0)
    normal = -normal;

  return normal;
}

void ApplyAreaFeature::BuildEdges(int vertexIndex1, int vertexIndex2, int vertexIndex3)
{
  // Check if triangle is degenerate.
  if (vertexIndex1 == vertexIndex2 || vertexIndex2 == vertexIndex3 || vertexIndex1 == vertexIndex3)
    return;

  TEdge edge1 = make_pair(vertexIndex1, vertexIndex2);
  if (!FindEdge(edge1))
    m_edges.push_back(make_pair(move(edge1), vertexIndex3));

  TEdge edge2 = make_pair(vertexIndex2, vertexIndex3);
  if (!FindEdge(edge2))
    m_edges.push_back(make_pair(move(edge2), vertexIndex1));

  TEdge edge3 = make_pair(vertexIndex3, vertexIndex1);
  if (!FindEdge(edge3))
    m_edges.push_back(make_pair(move(edge3), vertexIndex2));
}

void ApplyAreaFeature::CalculateBuildingEdges(vector<BuildingEdge> & edges)
{
  for (auto & e : m_edges)
  {
    if (e.second < 0)
      continue;
    BuildingEdge edge;
    edge.m_startVertex = m_indices[e.first.first];
    edge.m_endVertex = m_indices[e.first.second];
    edge.m_normal = CalculateNormal(edge.m_startVertex, edge.m_endVertex, m_indices[e.second]);
    edges.push_back(move(edge));
  }
}

void ApplyAreaFeature::ProcessRule(Stylist::TRuleWrapper const & rule)
{
  drule::BaseRule const * pRule = rule.first;
  double const depth = rule.second;

  AreaRuleProto const * areaRule = pRule->GetArea();
  if (areaRule && !m_triangles.empty())
  {
    AreaViewParams params;
    params.m_depth = depth;
    params.m_color = ToDrapeColor(areaRule->color());
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_minPosZ = m_minPosZ;
    params.m_posZ = m_posZ;

    vector<BuildingEdge> edges;
    if (m_isBuilding)
    {
      edges.reserve(m_edges.size());
      CalculateBuildingEdges(edges);
    }

    m_insertShape(make_unique_dp<AreaShape>(move(m_triangles), move(edges), params));
  }
  else
    TBase::ProcessRule(rule);
}

ApplyLineFeature::ApplyLineFeature(TInsertShapeFn const & insertShape, FeatureID const & id,
                                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                                   double currentScaleGtoP, bool simplify, size_t pointsCount)
  : TBase(insertShape, id, minVisibleScale, rank, captions)
  , m_currentScaleGtoP(currentScaleGtoP)
  , m_sqrScale(math::sqr(m_currentScaleGtoP))
  , m_simplify(simplify)
  , m_initialPointsCount(pointsCount)
  , m_shieldDepth(0.0)
  , m_shieldRule(nullptr)
#ifdef CALC_FILTERED_POINTS
  , m_readedCount(0)
#endif
{
}

void ApplyLineFeature::operator() (m2::PointD const & point)
{
#ifdef CALC_FILTERED_POINTS
  ++m_readedCount;
#endif

  if (m_spline.IsNull())
    m_spline.Reset(new m2::Spline(m_initialPointsCount));

  if (m_spline->IsEmpty())
  {
    m_spline->AddPoint(point);
    m_lastAddedPoint = point;
  }
  else
  {
    static float minSegmentLength = math::sqr(4.0 * df::VisualParams::Instance().GetVisualScale());
    if (m_simplify &&
        ((m_spline->GetSize() > 1 && point.SquareLength(m_lastAddedPoint) * m_sqrScale < minSegmentLength) ||
        m_spline->IsPrelonging(point)))
    {
      m_spline->ReplacePoint(point);
    }
    else
    {
      m_spline->AddPoint(point);
      m_lastAddedPoint = point;
    }
  }
}

bool ApplyLineFeature::HasGeometry() const
{
  return m_spline->IsValid();
}

void ApplyLineFeature::ProcessRule(Stylist::TRuleWrapper const & rule)
{
  ASSERT(HasGeometry(), ());
  drule::BaseRule const * pRule = rule.first;
  float depth = rule.second;

  bool isWay = (pRule->GetType() & drule::way) != 0;
  CaptionDefProto const * pCaptionRule = pRule->GetCaption(0);
  LineDefProto const * pLineRule = pRule->GetLine();
  ShieldRuleProto const * pShieldRule = pRule->GetShield();

  if (pCaptionRule != nullptr && pCaptionRule->height() > 2 &&
      !m_captions.GetPathName().empty() && isWay)
  {
    dp::FontDecl fontDecl;
    CaptionDefProtoToFontDecl(pCaptionRule, fontDecl);

    PathTextViewParams params;
    params.m_depth = depth;
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_text = m_captions.GetPathName();
    params.m_textFont = fontDecl;
    params.m_baseGtoPScale = m_currentScaleGtoP;

    m_insertShape(make_unique_dp<PathTextShape>(m_spline, params));
  }

  if (pLineRule != nullptr)
  {
    if (pLineRule->has_pathsym())
    {
      PathSymProto const & symRule = pLineRule->pathsym();
      PathSymbolViewParams params;
      params.m_depth = depth;
      params.m_minVisibleScale = m_minVisibleScale;
      params.m_rank = m_rank;
      params.m_symbolName = symRule.name();
      float const mainScale = df::VisualParams::Instance().GetVisualScale();
      params.m_offset = symRule.offset() * mainScale;
      params.m_step = symRule.step() * mainScale;
      params.m_baseGtoPScale = m_currentScaleGtoP;

      m_insertShape(make_unique_dp<PathSymbolShape>(m_spline, params));
    }
    else
    {
      LineViewParams params;
      Extract(pLineRule, params);
      params.m_depth = depth;
      params.m_minVisibleScale = m_minVisibleScale;
      params.m_rank = m_rank;
      params.m_baseGtoPScale = m_currentScaleGtoP;

      m_insertShape(make_unique_dp<LineShape>(m_spline, params));
    }
  }

  if (pShieldRule != nullptr)
  {
    m_shieldDepth = depth;
    m_shieldRule = pShieldRule;
  }
}

void ApplyLineFeature::Finish()
{
#ifdef CALC_FILTERED_POINTS
  LinesStat::Get().InsertLine(m_id, m_currentScaleGtoP, m_readedCount, m_spline->GetSize());
#endif

  if (m_shieldRule == nullptr)
    return;

  string const & roadNumber = m_captions.GetRoadNumber();
  if (roadNumber.empty())
    return;

  dp::FontDecl font;
  ShieldRuleProtoToFontDecl(m_shieldRule, font);

  double const pathPixelLength = m_spline->GetLength() * m_currentScaleGtoP;
  int const textHeight = static_cast<int>(font.m_size);

  // I don't know why we draw by this, but it's work before and will work now
  if (pathPixelLength > (roadNumber.size() + 2) * textHeight)
  {
    // TODO in future we need to choose emptySpace according GtoP scale.
    double const emptySpace = 1000.0;
    int const count = static_cast<int>((pathPixelLength / emptySpace) + 2);
    double const splineStep = pathPixelLength / count;

    float const mainScale = df::VisualParams::Instance().GetVisualScale();

    TextViewParams viewParams;
    viewParams.m_depth = m_shieldDepth;
    viewParams.m_minVisibleScale = m_minVisibleScale;
    viewParams.m_rank = m_rank;
    viewParams.m_anchor = dp::Center;
    viewParams.m_featureID = FeatureID();
    viewParams.m_primaryText = roadNumber;
    viewParams.m_primaryTextFont = font;
    viewParams.m_primaryOffset = m2::PointF(0, 0);
    viewParams.m_primaryOptional = true;
    viewParams.m_secondaryOptional = true;
    viewParams.m_extendingSize = m_shieldRule->has_min_distance() ? mainScale * m_shieldRule->min_distance() : 0;

    m2::Spline::iterator it = m_spline.CreateIterator();
    while (!it.BeginAgain())
    {
      m_insertShape(make_unique_dp<TextShape>(it.m_pos, viewParams, false /* hasPOI */,
                                              false /* affectedByZoomPriority */));
      it.Advance(splineStep);
    }
  }
}

} // namespace df
