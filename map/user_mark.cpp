#include "map/user_mark.hpp"
#include "map/user_mark_layer.hpp"

#include "indexer/classificator.hpp"

#include "geometry/mercator.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include <atomic>

namespace
{
static const uint32_t kMarkIdTypeBitsCount = 4;
static const std::string kLastBookmarkId = "LastBookmarkId";

uint64_t LoadLastBookmarkId()
{
  uint64_t lastId;
  std::string val;
  if (GetPlatform().GetSecureStorage().Load(kLastBookmarkId, val) && strings::to_uint64(val, lastId))
    return lastId;
  return 0;
}

void SaveLastBookmarkId(uint64_t lastId)
{
  GetPlatform().GetSecureStorage().Save(kLastBookmarkId, strings::to_string(lastId));
}

kml::MarkId GetNextUserMarkId(UserMark::Type type, bool reset = false)
{
  static std::atomic<uint64_t> lastBookmarkId(LoadLastBookmarkId());
  static std::atomic<uint64_t> lastUserMarkId(0);

  if (reset)
  {
    if (type == UserMark::Type::BOOKMARK)
    {
      SaveLastBookmarkId(0);
      lastBookmarkId = 0;
    }
    return kml::kInvalidMarkId;
  }

  static_assert(UserMark::Type::COUNT <= (1 << kMarkIdTypeBitsCount), "Not enough bits for user mark type.");

  auto const typeBits = static_cast<uint64_t>(type) << (sizeof(kml::MarkId) * 8 - kMarkIdTypeBitsCount);
  if (type == UserMark::Type::BOOKMARK)
  {
    auto const id = static_cast<kml::MarkId>((++lastBookmarkId) | typeBits);
    SaveLastBookmarkId(lastBookmarkId);
    return id;
  }
  else
  {
    return static_cast<kml::MarkId>((++lastUserMarkId) | typeBits);
  }
}
}  // namespace

UserMark::UserMark(kml::MarkId id, m2::PointD const & ptOrg, UserMark::Type type)
  : df::UserPointMark(id == kml::kInvalidMarkId ? GetNextUserMarkId(type) : id)
  , m_ptOrg(ptOrg)
{
  ASSERT_EQUAL(GetMarkType(), type, ());
}

UserMark::UserMark(m2::PointD const & ptOrg, UserMark::Type type)
  : df::UserPointMark(GetNextUserMarkId(type))
  , m_ptOrg(ptOrg)
{}

// static
UserMark::Type UserMark::GetMarkType(kml::MarkId id)
{
  return static_cast<Type>(id >> (sizeof(id) * 8 - kMarkIdTypeBitsCount));
}

// static
void UserMark::ResetLastId(UserMark::Type type)
{
  UNUSED_VALUE(GetNextUserMarkId(type, true /* reset */));
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
  case UserMark::Type::COUNT: return "COUNT";
  }
}
