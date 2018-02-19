#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <atomic>

namespace
{
static const uint32_t kMarkIdTypeBitsCount = 4;

df::MarkID GetNextUserMarkId(UserMark::Type type)
{
  static std::atomic<uint32_t> nextMarkId(0);

  static_assert(UserMark::Type::BOOKMARK < (1 << kMarkIdTypeBitsCount), "Not enough bits for user mark type.");
  return static_cast<df::MarkID>(
    (++nextMarkId) | (type << static_cast<uint32_t>(sizeof(df::MarkID) * 8 - kMarkIdTypeBitsCount)));
}
}  // namespace

UserMark::UserMark(m2::PointD const & ptOrg, UserMark::Type type)
  : df::UserPointMark(GetNextUserMarkId(type))
  , m_ptOrg(ptOrg)
{}

// static
UserMark::Type UserMark::GetMarkType(df::MarkID id)
{
  return static_cast<Type>(id >> (sizeof(id) * 8 - kMarkIdTypeBitsCount));
}

m2::PointD const & UserMark::GetPivot() const
{
  return m_ptOrg;
}

m2::PointD UserMark::GetPixelOffset() const
{
  return {};
}

dp::Anchor UserMark::GetAnchor() const
{
  return dp::Center;
}

df::RenderState::DepthLayer UserMark::GetDepthLayer() const
{
  return df::RenderState::UserMarkLayer;
}

ms::LatLon UserMark::GetLatLon() const
{
  return MercatorBounds::ToLatLon(m_ptOrg);
}

StaticMarkPoint::StaticMarkPoint(m2::PointD const & ptOrg)
  : UserMark(ptOrg, UserMark::Type::STATIC)
{}

void StaticMarkPoint::SetPtOrg(m2::PointD const & ptOrg)
{
  SetDirty();
  m_ptOrg = ptOrg;
}

MyPositionMarkPoint::MyPositionMarkPoint(m2::PointD const & ptOrg)
  : StaticMarkPoint(ptOrg)
{}

DebugMarkPoint::DebugMarkPoint(const m2::PointD & ptOrg)
  : UserMark(ptOrg, UserMark::Type::DEBUG_MARK)
{}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> DebugMarkPoint::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "api-result"));
  return symbol;
}

string DebugPrint(UserMark::Type type)
{
  switch (type)
  {
  case UserMark::Type::API: return "API";
  case UserMark::Type::SEARCH: return "SEARCH";
  case UserMark::Type::STATIC: return "STATIC";
  case UserMark::Type::BOOKMARK: return "BOOKMARK";
  case UserMark::Type::DEBUG_MARK: return "DEBUG_MARK";
  case UserMark::Type::ROUTING: return "ROUTING";
  case UserMark::Type::LOCAL_ADS: return "LOCAL_ADS";
  case UserMark::Type::TRANSIT: return "TRANSIT";
  }
}
