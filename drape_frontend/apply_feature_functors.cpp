#include "drape_frontend/apply_feature_functors.hpp"

#include "drape_frontend/area_shape.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/colored_symbol_shape.hpp"
#include "drape_frontend/line_shape.hpp"
#include "drape_frontend/path_symbol_shape.hpp"
#include "drape_frontend/path_text_shape.hpp"
#include "drape_frontend/poi_symbol_shape.hpp"
#include "drape_frontend/text_shape.hpp"
#include "drape_frontend/visual_params.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/osm_editor.hpp"

#include "geometry/clipping.hpp"

#include "drape/color.hpp"
#include "drape/stipple_pen_resource.hpp"
#include "drape/utils/projection.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace df
{

namespace
{
double const kMinVisibleFontSize = 8.0;

string const kStarSymbol = "★";
string const kPriceSymbol = "$";

int const kLineSimplifyLevelStart = 10;
int const kLineSimplifyLevelEnd = 12;

double const kPathTextBaseTextIndex = 0;
double const kPathTextBaseTextStep = 100;
double const kShieldBaseTextIndex = 1000;

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

void Extract(::LineDefProto const * lineRule, df::LineViewParams & params)
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

void CaptionDefProtoToFontDecl(CaptionDefProto const * capRule, dp::FontDecl & params)
{
  double const vs = df::VisualParams::Instance().GetVisualScale();
  params.m_color = ToDrapeColor(capRule->color());
  params.m_size = max(kMinVisibleFontSize, capRule->height() * vs);

  if (capRule->has_stroke_color())
    params.m_outlineColor = ToDrapeColor(capRule->stroke_color());
  else if (vs < df::VisualParams::kHdpiScale)
    params.m_isSdf = false;
}

void ShieldRuleProtoToFontDecl(ShieldRuleProto const * shieldRule, dp::FontDecl & params)
{
  double const vs = df::VisualParams::Instance().GetVisualScale();
  params.m_color = ToDrapeColor(shieldRule->text_color());
  params.m_size = max(kMinVisibleFontSize, shieldRule->height() * vs);
  if (shieldRule->has_text_stroke_color())
   params.m_outlineColor = ToDrapeColor(shieldRule->text_stroke_color());

  if (vs < df::VisualParams::kHdpiScale)
    params.m_isSdf = false;
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

uint16_t CalculateHotelOverlayPriority(BaseApplyFeature::HotelData const & data)
{
  // NOTE: m_rating is in format X[.Y], where X = [0;10], Y = [0;9], e.g. 8.7
  string s = data.m_rating;
  s.erase(remove(s.begin(), s.end(), '.'), s.end());
  s.erase(remove(s.begin(), s.end(), ','), s.end());
  if (s.empty())
    return 0;

  // Special case for integer ratings.
  if (s.length() == data.m_rating.length())
    s += '0';

  uint result = 0;
  if (strings::to_uint(s, result))
    return static_cast<uint16_t>(result);
  return 0;
}

bool IsSymbolRoadShield(ftypes::RoadShield const & shield)
{
  return shield.m_type == ftypes::RoadShieldType::US_Interstate ||
         shield.m_type == ftypes::RoadShieldType::US_Highway;
}

std::string GetRoadShieldSymbolName(ftypes::RoadShield const & shield, double fontScale)
{
  std::string result = "";
  if (shield.m_type == ftypes::RoadShieldType::US_Interstate)
    result = shield.m_name.size() <= 2 ? "shield-us-i-thin" : "shield-us-i-wide";
  else if (shield.m_type == ftypes::RoadShieldType::US_Highway)
    result = shield.m_name.size() <= 2 ? "shield-us-hw-thin" : "shield-us-hw-wide";

  if (!result.empty() && fontScale > 1.0)
    result += "-scaled";

  return result;
}

bool IsColoredRoadShield(ftypes::RoadShield const & shield)
{
  return shield.m_type == ftypes::RoadShieldType::Default ||
         shield.m_type == ftypes::RoadShieldType::UK_Highway ||
         shield.m_type == ftypes::RoadShieldType::UK_Motorway ||
         shield.m_type == ftypes::RoadShieldType::Russia_Highway ||
         shield.m_type == ftypes::RoadShieldType::Euro_Motorway ||
         shield.m_type == ftypes::RoadShieldType::France_Motorway ||
         shield.m_type == ftypes::RoadShieldType::France_Departmental;
}

dp::FontDecl GetRoadShieldTextFont(MapStyle const & style, dp::FontDecl const & baseFont,
                                   ftypes::RoadShield const & shield)
{
  dp::FontDecl f = baseFont;
  f.m_outlineColor = dp::Color::Transparent();

  using ftypes::RoadShieldType;

  static std::unordered_map<int, df::ColorConstant> kColors = {
    {static_cast<int>(RoadShieldType::Euro_Motorway), df::RoadShieldWhiteText},
    {static_cast<int>(RoadShieldType::UK_Motorway), df::RoadShieldWhiteText},
    {static_cast<int>(RoadShieldType::UK_Highway), df::RoadShieldUKYellowText},
    {static_cast<int>(RoadShieldType::US_Interstate), df::RoadShieldWhiteText},
    {static_cast<int>(RoadShieldType::US_Highway), df::RoadShieldBlackText},
    {static_cast<int>(RoadShieldType::Russia_Highway), df::RoadShieldWhiteText},
    {static_cast<int>(RoadShieldType::France_Motorway), df::RoadShieldWhiteText},
    {static_cast<int>(RoadShieldType::France_Departmental), df::RoadShieldBlackText}
  };

  auto it = kColors.find(static_cast<int>(shield.m_type));
  if (it != kColors.end())
    f.m_color = df::GetColorConstant(style, it->second);

  return f;
}

dp::Color GetRoadShieldColor(MapStyle const & style, dp::Color const & baseColor,
                             ftypes::RoadShield const & shield)
{
  using ftypes::RoadShieldType;

  static std::unordered_map<int, df::ColorConstant> kColors = {
    {static_cast<int>(RoadShieldType::Euro_Motorway), df::RoadShieldGreenBackground},
    {static_cast<int>(RoadShieldType::UK_Motorway), df::RoadShieldBlueBackground},
    {static_cast<int>(RoadShieldType::UK_Highway), df::RoadShieldGreenBackground},
    {static_cast<int>(RoadShieldType::Russia_Highway), df::RoadShieldBlueBackground},
    {static_cast<int>(RoadShieldType::France_Motorway), df::RoadShieldRedBackground},
    {static_cast<int>(RoadShieldType::France_Departmental), df::RoadShieldOrangeBackground}
  };

  auto it = kColors.find(static_cast<int>(shield.m_type));
  if (it != kColors.end())
    return df::GetColorConstant(style, it->second);

  return baseColor;
}

float GetRoadShieldOutlineWidth(float baseWidth, ftypes::RoadShield const & shield)
{
  if (shield.m_type == ftypes::RoadShieldType::UK_Highway ||
      shield.m_type == ftypes::RoadShieldType::UK_Motorway ||
      shield.m_type == ftypes::RoadShieldType::Russia_Highway ||
      shield.m_type == ftypes::RoadShieldType::Euro_Motorway ||
      shield.m_type == ftypes::RoadShieldType::France_Motorway)
    return 0.0f;

  return baseWidth;
}
} //  namespace

BaseApplyFeature::BaseApplyFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                                   int minVisibleScale, uint8_t rank, CaptionDescription const & captions)
  : m_insertShape(insertShape)
  , m_id(id)
  , m_captions(captions)
  , m_minVisibleScale(minVisibleScale)
  , m_rank(rank)
  , m_tileKey(tileKey)
  , m_tileRect(tileKey.GetGlobalRect())
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

string BaseApplyFeature::ExtractHotelInfo() const
{
  if (!m_hotelData.m_isHotel)
    return "";

  ostringstream out;
  if (!m_hotelData.m_rating.empty() && m_hotelData.m_rating != "0")
  {
    out << m_hotelData.m_rating << kStarSymbol;
    if (m_hotelData.m_priceCategory != 0)
      out << "  ";
  }
  for (int i = 0; i < m_hotelData.m_priceCategory; i++)
    out << kPriceSymbol;

  return out.str();
}

void BaseApplyFeature::SetHotelData(HotelData && hotelData)
{
  m_hotelData = move(hotelData);
}

ApplyPointFeature::ApplyPointFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                                     int minVisibleScale, uint8_t rank, CaptionDescription const & captions,
                                     float posZ)
  : TBase(tileKey, insertShape, id, minVisibleScale, rank, captions)
  , m_posZ(posZ)
  , m_hasPoint(false)
  , m_hasArea(false)
  , m_createdByEditor(false)
  , m_obsoleteInEditor(false)
  , m_symbolDepth(dp::minDepth)
  , m_symbolRule(nullptr)
{
}

void ApplyPointFeature::operator()(m2::PointD const & point, bool hasArea)
{
  auto const & editor = osm::Editor::Instance();
  m_hasPoint = true;
  m_hasArea = hasArea;
  auto const featureStatus = editor.GetFeatureStatus(m_id);
  m_createdByEditor = featureStatus == osm::Editor::FeatureStatus::Created;
  m_obsoleteInEditor = featureStatus == osm::Editor::FeatureStatus::Obsolete;
  m_centerPoint = point;
}

void ApplyPointFeature::ProcessRule(Stylist::TRuleWrapper const & rule)
{
  if (!m_hasPoint)
    return;

  drule::BaseRule const * pRule = rule.first;
  float const depth = rule.second;

  SymbolRuleProto const * symRule = pRule->GetSymbol();
  if (symRule != nullptr)
  {
    m_symbolDepth = depth;
    m_symbolRule = symRule;
  }

  bool const hasPOI = m_symbolRule != nullptr;
  bool const isNode = (pRule->GetType() & drule::node) != 0;
  CaptionDefProto const * capRule = pRule->GetCaption(0);
  if (capRule && isNode)
  {
    TextViewParams params;
    params.m_tileCenter = m_tileRect.Center();
    ExtractCaptionParams(capRule, pRule->GetCaption(1), depth, params);
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_posZ = m_posZ;
    params.m_hasArea = m_hasArea;
    params.m_createdByEditor = m_createdByEditor;
    if (!params.m_primaryText.empty() || !params.m_secondaryText.empty())
    {
      int displacementMode = dp::displacement::kAllModes;
      // For hotels we set only kDefaultMode, because we have a special shape
      // for kHotelMode and this shape will not be displayed in this case.
      if (m_hotelData.m_isHotel)
        displacementMode = dp::displacement::kDefaultMode;
      m_insertShape(make_unique_dp<TextShape>(m_centerPoint, params, m_tileKey,
                                              hasPOI, 0 /* textIndex */,
                                              true /* affectedByZoomPriority */,
                                              displacementMode));
    }
    if (m_hotelData.m_isHotel && !params.m_primaryText.empty())
    {
      params.m_primaryOptional = false;
      params.m_primaryTextFont.m_size *= 1.2;
      auto const style = GetStyleReader().GetCurrentStyle();
      params.m_primaryTextFont.m_outlineColor = df::GetColorConstant(style, df::PoiHotelTextOutline);
      params.m_secondaryTextFont = params.m_primaryTextFont;
      params.m_secondaryText = ExtractHotelInfo();
      params.m_secondaryOptional = false;
      uint16_t const priority = CalculateHotelOverlayPriority(m_hotelData);
      m_insertShape(make_unique_dp<TextShape>(m_centerPoint, params, m_tileKey,
                                              hasPOI, 0 /* textIndex */,
                                              true /* affectedByZoomPriority */,
                                              dp::displacement::kHotelMode, priority));
    }
  }
}

void ApplyPointFeature::Finish()
{
  if (m_symbolRule == nullptr)
    return;

  PoiSymbolViewParams params(m_id);
  params.m_tileCenter = m_tileRect.Center();
  params.m_depth = m_symbolDepth;
  params.m_minVisibleScale = m_minVisibleScale;
  params.m_rank = m_rank;
  params.m_symbolName = m_symbolRule->name();
  float const mainScale = df::VisualParams::Instance().GetVisualScale();
  params.m_extendingSize = m_symbolRule->has_min_distance() ? mainScale * m_symbolRule->min_distance() : 0;
  params.m_posZ = m_posZ;
  params.m_hasArea = m_hasArea;
  params.m_createdByEditor = m_createdByEditor;
  params.m_obsoleteInEditor = m_obsoleteInEditor;

  m_insertShape(make_unique_dp<PoiSymbolShape>(m_centerPoint, params, m_tileKey, 0 /* text index */,
                                               m_hotelData.m_isHotel ? dp::displacement::kDefaultMode :
                                                                       dp::displacement::kAllModes));
  if (m_hotelData.m_isHotel)
  {
    uint16_t const priority = CalculateHotelOverlayPriority(m_hotelData);
    m_insertShape(make_unique_dp<PoiSymbolShape>(m_centerPoint, params, m_tileKey, 0 /* text index */,
                                                 dp::displacement::kHotelMode, priority));
  }
}

ApplyAreaFeature::ApplyAreaFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                                   bool isBuilding, float minPosZ, float posZ, int minVisibleScale,
                                   uint8_t rank, bool generateOutline, CaptionDescription const & captions)
  : TBase(tileKey, insertShape, id, minVisibleScale, rank, captions, posZ)
  , m_minPosZ(minPosZ)
  , m_isBuilding(isBuilding)
  , m_generateOutline(generateOutline)
{}

void ApplyAreaFeature::operator()(m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
{
  if (m_isBuilding)
  {
    ProcessBuildingPolygon(p1, p2, p3);
    return;
  }

  m2::PointD const v1 = p2 - p1;
  m2::PointD const v2 = p3 - p1;
  if (v1.IsAlmostZero() || v2.IsAlmostZero())
    return;

  double const crossProduct = m2::CrossProduct(v1.Normalize(), v2.Normalize());
  double const kEps = 1e-7;
  if (fabs(crossProduct) < kEps)
    return;

  auto const clipFunctor = [this](m2::PointD const & p1, m2::PointD const & p2, m2::PointD const & p3)
  {
    m_triangles.push_back(p1);
    m_triangles.push_back(p2);
    m_triangles.push_back(p3);
  };

  if (m2::CrossProduct(p2 - p1, p3 - p1) < 0)
    m2::ClipTriangleByRect(m_tileRect, p1, p2, p3, clipFunctor);
  else
    m2::ClipTriangleByRect(m_tileRect, p1, p3, p2, clipFunctor);
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

  if (crossProduct < 0)
  {
    m_triangles.push_back(p1);
    m_triangles.push_back(p2);
    m_triangles.push_back(p3);
    BuildEdges(GetIndex(p1), GetIndex(p2), GetIndex(p3));
  }
  else
  {
    m_triangles.push_back(p1);
    m_triangles.push_back(p3);
    m_triangles.push_back(p2);
    BuildEdges(GetIndex(p1), GetIndex(p3), GetIndex(p2));
  }
}

int ApplyAreaFeature::GetIndex(m2::PointD const & pt)
{
  for (size_t i = 0; i < m_points.size(); i++)
  {
    if (pt.EqualDxDy(m_points[i], 1e-7))
      return static_cast<int>(i);
  }
  m_points.push_back(pt);
  return static_cast<int>(m_points.size()) - 1;
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

void ApplyAreaFeature::CalculateBuildingOutline(bool calculateNormals, BuildingOutline & outline)
{
  outline.m_vertices = move(m_points);
  outline.m_indices.reserve(m_edges.size() * 2);
  if (calculateNormals)
    outline.m_normals.reserve(m_edges.size());

  for (auto & e : m_edges)
  {
    if (e.second < 0)
      continue;

    outline.m_indices.push_back(e.first.first);
    outline.m_indices.push_back(e.first.second);

    if (calculateNormals)
    {
      outline.m_normals.emplace_back(CalculateNormal(outline.m_vertices[e.first.first],
                                                     outline.m_vertices[e.first.second],
                                                     outline.m_vertices[e.second]));
    }
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
    params.m_tileCenter = m_tileRect.Center();
    params.m_depth = depth;
    params.m_color = ToDrapeColor(areaRule->color());
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_minPosZ = m_minPosZ;
    params.m_posZ = m_posZ;

    BuildingOutline outline;
    if (m_isBuilding)
    {
      outline.m_generateOutline = m_generateOutline;
      bool const calculateNormals = m_posZ > 0.0;
      CalculateBuildingOutline(calculateNormals, outline);
      params.m_is3D = !outline.m_indices.empty() && calculateNormals;
    }

    m_insertShape(make_unique_dp<AreaShape>(move(m_triangles), move(outline), params));
  }
  else
  {
    TBase::ProcessRule(rule);
  }
}

ApplyLineFeature::ApplyLineFeature(TileKey const & tileKey, TInsertShapeFn const & insertShape, FeatureID const & id,
                                   double currentScaleGtoP, int minVisibleScale, uint8_t rank,
                                   CaptionDescription const & captions, size_t pointsCount)
  : TBase(tileKey, insertShape, id, minVisibleScale, rank, captions)
  , m_currentScaleGtoP(currentScaleGtoP)
  , m_sqrScale(math::sqr(m_currentScaleGtoP))
  , m_simplify(tileKey.m_zoomLevel >= kLineSimplifyLevelStart && tileKey.m_zoomLevel <= kLineSimplifyLevelEnd)
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

  m_clippedSplines = m2::ClipSplineByRect(m_tileRect, m_spline);

  if (m_clippedSplines.empty())
    return;

  if (pCaptionRule != nullptr && pCaptionRule->height() > 2 &&
      !m_captions.GetPathName().empty() && isWay)
  {
    dp::FontDecl fontDecl;
    CaptionDefProtoToFontDecl(pCaptionRule, fontDecl);

    PathTextViewParams params;
    params.m_tileCenter = m_tileRect.Center();
    params.m_featureID = m_id;
    params.m_depth = depth;
    params.m_minVisibleScale = m_minVisibleScale;
    params.m_rank = m_rank;
    params.m_text = m_captions.GetPathName();
    params.m_textFont = fontDecl;
    params.m_baseGtoPScale = m_currentScaleGtoP;

    uint32_t baseTextIndex = kPathTextBaseTextIndex;
    for (auto const & spline : m_clippedSplines)
    {
      m_insertShape(make_unique_dp<PathTextShape>(spline, params, m_tileKey, baseTextIndex));
      baseTextIndex += kPathTextBaseTextStep;
    }
  }

  if (pLineRule != nullptr)
  {
    if (pLineRule->has_pathsym())
    {
      PathSymProto const & symRule = pLineRule->pathsym();
      PathSymbolViewParams params;
      params.m_tileCenter = m_tileRect.Center();
      params.m_depth = depth;
      params.m_minVisibleScale = m_minVisibleScale;
      params.m_rank = m_rank;
      params.m_symbolName = symRule.name();
      float const mainScale = df::VisualParams::Instance().GetVisualScale();
      params.m_offset = symRule.offset() * mainScale;
      params.m_step = symRule.step() * mainScale;
      params.m_baseGtoPScale = m_currentScaleGtoP;

      for (auto const & spline : m_clippedSplines)
        m_insertShape(make_unique_dp<PathSymbolShape>(spline, params));
    }
    else
    {
      LineViewParams params;
      params.m_tileCenter = m_tileRect.Center();
      Extract(pLineRule, params);
      params.m_depth = depth;
      params.m_minVisibleScale = m_minVisibleScale;
      params.m_rank = m_rank;
      params.m_baseGtoPScale = m_currentScaleGtoP;
      params.m_zoomLevel = m_tileKey.m_zoomLevel;

      for (auto const & spline : m_clippedSplines)
        m_insertShape(make_unique_dp<LineShape>(spline, params));
    }
  }

  if (pShieldRule != nullptr)
  {
    m_shieldDepth = depth;
    m_shieldRule = pShieldRule;
  }
}

void ApplyLineFeature::GetRoadShieldsViewParams(ftypes::RoadShield const & shield, TextViewParams & textParams,
                                                ColoredSymbolViewParams & symbolParams,
                                                PoiSymbolViewParams & poiParams)
{
  ASSERT (m_shieldRule != nullptr, ());

  string const & roadNumber = shield.m_name;
  float const mainScale = df::VisualParams::Instance().GetVisualScale();
  auto const style = GetStyleReader().GetCurrentStyle();
  double const fontScale = df::VisualParams::Instance().GetFontScale();

  // Text properties.
  dp::FontDecl baseFont;
  ShieldRuleProtoToFontDecl(m_shieldRule, baseFont);
  dp::FontDecl font = GetRoadShieldTextFont(style, baseFont, shield);
  textParams.m_tileCenter = m_tileRect.Center();
  textParams.m_depth = m_shieldDepth;
  textParams.m_minVisibleScale = m_minVisibleScale;
  textParams.m_rank = m_rank;
  textParams.m_anchor = dp::Center;
  textParams.m_featureID = m_id;
  textParams.m_primaryText = roadNumber;
  textParams.m_primaryTextFont = font;
  textParams.m_primaryOffset = m2::PointF(0, 0);
  textParams.m_primaryOptional = false;
  textParams.m_secondaryOptional = false;
  textParams.m_extendingSize = 0;

  // Calculate width and height of a shield.
  float const shieldWidth = (font.m_size * 0.5 * roadNumber.size() + 10.0f * mainScale) * fontScale;
  float const shieldHeight = (font.m_size + 3.0f * mainScale) * fontScale;
  textParams.m_limitedText = true;
  textParams.m_limits = m2::PointF(shieldWidth, shieldHeight) * 0.9f;

  if (IsColoredRoadShield(shield))
  {
    // Generated symbol properties.
    symbolParams.m_featureID = m_id;
    symbolParams.m_tileCenter = m_tileRect.Center();
    symbolParams.m_depth = m_shieldDepth;
    symbolParams.m_minVisibleScale = m_minVisibleScale;
    symbolParams.m_rank = m_rank;
    symbolParams.m_shape = ColoredSymbolViewParams::Shape::RoundedRectangle;
    symbolParams.m_radiusInPixels = 2.5f * mainScale;
    symbolParams.m_color = ToDrapeColor(m_shieldRule->color());
    if (m_shieldRule->has_stroke_color())
    {
      symbolParams.m_outlineColor = ToDrapeColor(m_shieldRule->stroke_color());
      symbolParams.m_outlineWidth = 1.0f * mainScale;
    }
    symbolParams.m_sizeInPixels = m2::PointF(shieldWidth, shieldHeight);
    symbolParams.m_outlineWidth = GetRoadShieldOutlineWidth(symbolParams.m_outlineWidth, shield);
    symbolParams.m_color = GetRoadShieldColor(style, symbolParams.m_color, shield);
  }

  // Image symbol properties.
  if (IsSymbolRoadShield(shield))
  {
    std::string symbolName = GetRoadShieldSymbolName(shield, fontScale);
    if (!symbolName.empty() && !shield.m_additionalText.empty())
    {
      textParams.m_anchor = dp::Top;
      textParams.m_secondaryText = shield.m_additionalText;
      textParams.m_secondaryTextFont = textParams.m_primaryTextFont;
      textParams.m_secondaryTextFont.m_color = df::GetColorConstant(style, df::RoadShieldBlackText);
      textParams.m_secondaryTextFont.m_outlineColor = df::GetColorConstant(style, df::RoadShieldWhiteText);
      textParams.m_primaryOffset = m2::PointF(0.0f, -0.5f * textParams.m_primaryTextFont.m_size);
      textParams.m_secondaryTextFont.m_size *= 0.9f;
      textParams.m_secondaryOffset = m2::PointF(0.0f, 3.0f * mainScale);
    }

    poiParams.m_tileCenter = m_tileRect.Center();
    poiParams.m_depth = m_shieldDepth;
    poiParams.m_minVisibleScale = m_minVisibleScale;
    poiParams.m_rank = m_rank;
    poiParams.m_symbolName = symbolName;
    poiParams.m_extendingSize = 0;
    poiParams.m_posZ = 0.0f;
    poiParams.m_hasArea = false;
    poiParams.m_createdByEditor = false;
    poiParams.m_obsoleteInEditor = false;
  }
}

void ApplyLineFeature::Finish(std::vector<ftypes::RoadShield> && roadShields)
{
#ifdef CALC_FILTERED_POINTS
  LinesStat::Get().InsertLine(m_id, m_currentScaleGtoP, m_readedCount, m_spline->GetSize());
#endif

  if (m_shieldRule == nullptr || m_clippedSplines.empty())
    return;

  uint32_t constexpr kDefaultMinDistance = 50;
  float const mainScale = df::VisualParams::Instance().GetVisualScale();

  m2::PointD shieldOffset(0.0, 0.0);
  for (size_t shieldIndex = 0; shieldIndex < roadShields.size(); shieldIndex++)
  {
    ftypes::RoadShield const & shield = roadShields[shieldIndex];
    TextViewParams textParams;
    ColoredSymbolViewParams symbolParams;
    PoiSymbolViewParams poiParams(m_id);
    GetRoadShieldsViewParams(shield, textParams, symbolParams, poiParams);

    uint32_t minDistanceInPixels = mainScale * (m_shieldRule->has_min_distance() ?
                                                m_shieldRule->min_distance() : kDefaultMinDistance);
    if (minDistanceInPixels == 0)
      minDistanceInPixels = mainScale * kDefaultMinDistance;

    uint32_t textIndex = kShieldBaseTextIndex * (shieldIndex + 1);
    for (auto const & spline : m_clippedSplines)
    {
      double const pathPixelLength = spline->GetLength() * m_currentScaleGtoP;
      uint32_t const shieldsCount = static_cast<uint32_t>(pathPixelLength / minDistanceInPixels);
      if (shieldsCount == 0)
        continue;
      double const splineStep = spline->GetLength() / (shieldsCount + 1);
      m2::Spline::iterator it = spline.CreateIterator();
      it.Advance(splineStep);

      for (uint32_t i = 0; i < shieldsCount && !it.BeginAgain(); i++)
      {
        m_insertShape(make_unique_dp<TextShape>(shieldOffset + it.m_pos, textParams, m_tileKey,
                                                true /* hasPOI */, textIndex,
                                                false /* affectedByZoomPriority */));
        if (IsColoredRoadShield(shield))
        {
          m_insertShape(make_unique_dp<ColoredSymbolShape>(shieldOffset + it.m_pos, symbolParams, m_tileKey, textIndex));
        }
        else if (IsSymbolRoadShield(shield))
        {
          m_insertShape(make_unique_dp<PoiSymbolShape>(shieldOffset + it.m_pos, poiParams, m_tileKey, textIndex));
        }
        it.Advance(splineStep);
        textIndex++;
      }
    }

    shieldOffset += m2::PointD(symbolParams.m_sizeInPixels.x / m_currentScaleGtoP, 0.0);
  }
}

m2::PolylineD ApplyLineFeature::GetPolyline() const
{
  return HasGeometry() ? m2::PolylineD(m_spline->GetPath()) : m2::PolylineD();
}

} // namespace df
