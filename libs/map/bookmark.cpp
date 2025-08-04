#include "map/bookmark.hpp"
#include "map/bookmark_helpers.hpp"

#include "indexer/scales.hpp"

#include "base/string_utils.hpp"

namespace
{
std::string GetBookmarkIconType(kml::BookmarkIcon const & icon)
{
  switch (icon)
  {
  case kml::BookmarkIcon::None: return "default";
  case kml::BookmarkIcon::Hotel: return "hotel";
  case kml::BookmarkIcon::Animals: return "animals";
  case kml::BookmarkIcon::Buddhism: return "buddhism";
  case kml::BookmarkIcon::Building: return "building";
  case kml::BookmarkIcon::Christianity: return "christianity";
  case kml::BookmarkIcon::Entertainment: return "entertainment";
  case kml::BookmarkIcon::Exchange: return "exchange";
  case kml::BookmarkIcon::Food: return "restaurant";
  case kml::BookmarkIcon::Gas: return "gas";
  case kml::BookmarkIcon::Judaism: return "judaism";
  case kml::BookmarkIcon::Medicine: return "medicine";
  case kml::BookmarkIcon::Mountain: return "mountain";
  case kml::BookmarkIcon::Museum: return "museum";
  case kml::BookmarkIcon::Islam: return "islam";
  case kml::BookmarkIcon::Park: return "park";
  case kml::BookmarkIcon::Parking: return "parking";
  case kml::BookmarkIcon::Shop: return "shop";
  case kml::BookmarkIcon::Sights: return "sights";
  case kml::BookmarkIcon::Swim: return "swim";
  case kml::BookmarkIcon::Water: return "water";
  case kml::BookmarkIcon::Bar: return "bar";
  case kml::BookmarkIcon::Transport: return "transport";
  case kml::BookmarkIcon::Viewpoint: return "viewpoint";
  case kml::BookmarkIcon::Sport: return "sport";
  case kml::BookmarkIcon::Pub: return "pub";
  case kml::BookmarkIcon::Art: return "art";
  case kml::BookmarkIcon::Bank: return "bank";
  case kml::BookmarkIcon::Cafe: return "cafe";
  case kml::BookmarkIcon::Pharmacy: return "pharmacy";
  case kml::BookmarkIcon::Stadium: return "stadium";
  case kml::BookmarkIcon::Theatre: return "theatre";
  case kml::BookmarkIcon::Information: return "information";
  case kml::BookmarkIcon::ChargingStation: return "charging_station";
  case kml::BookmarkIcon::BicycleParking: return "bicycle_parking";
  case kml::BookmarkIcon::BicycleParkingCovered: return "bicycle_parking_covered";
  case kml::BookmarkIcon::BicycleRental: return "bicycle_rental";
  case kml::BookmarkIcon::FastFood: return "fast_food";
  case kml::BookmarkIcon::Count: ASSERT(false, ("Invalid bookmark icon type")); return {};
  }
  UNREACHABLE();
}

std::string const kCustomImageProperty = "CustomImage";
std::string const kHasElevationProfileProperty = "has_elevation_profile";
}  // namespace

Bookmark::Bookmark(m2::PointD const & ptOrg) : Base(ptOrg, UserMark::BOOKMARK), m_groupId(kml::kInvalidMarkGroupId)
{
  m_data.m_point = ptOrg;
  m_data.m_id = GetId();
}

Bookmark::Bookmark(kml::BookmarkData && data)
  : Base(data.m_id, data.m_point, UserMark::BOOKMARK)
  , m_data(std::move(data))
  , m_groupId(kml::kInvalidMarkGroupId)
{
  m_data.m_id = GetId();
}

void Bookmark::SetData(kml::BookmarkData const & data)
{
  SetDirty();
  m_data = data;
}

kml::BookmarkData const & Bookmark::GetData() const
{
  return m_data;
}

search::ReverseGeocoder::RegionAddress const & Bookmark::GetAddress() const
{
  return m_address;
}

void Bookmark::SetAddress(search::ReverseGeocoder::RegionAddress const & address)
{
  SetDirty();
  m_address = address;
}

void Bookmark::SetIsVisible(bool isVisible)
{
  SetDirty();
  m_isVisible = isVisible;
}

drape_ptr<df::UserPointMark::TitlesInfo> Bookmark::GetTitleDeclEx(settings::Placement p, dp::Color outlineColor) const
{
  if (p == settings::Placement::None)
    return nullptr;

  dp::TitleDecl title;
  switch (p)
  {
  case settings::Placement::Right: title.m_anchor = dp::Left; break;
  case settings::Placement::Bottom: title.m_anchor = dp::Top; break;
  default: UNREACHABLE();
  }

  title.m_primaryTextFont.m_color = df::GetColorConstant(GetColorConstant());
  title.m_primaryTextFont.m_outlineColor = outlineColor;
  title.m_primaryTextFont.m_size = 11;  // most frequent font size in styles
  title.m_primaryText = GetPreferredName();
  title.m_primaryOffset.x = 1;

  auto titles = make_unique_dp<TitlesInfo>();
  titles->push_back(std::move(title));
  return titles;
}

df::DepthLayer Bookmark::GetDepthLayerEx(settings::Placement p) const
{
  if (p == settings::Placement::None)
    return df::DepthLayer::UserMarkLayer;

  // Texts:
  // - UserMarkLayer, aren't visible at all
  // - RoutingMarkLayer, displaced by Feature's texts
  // - SearchMarkLayer, overlapped with each other :)
  return df::DepthLayer::SearchMarkLayer;
}

dp::Anchor Bookmark::GetAnchor() const
{
  return dp::Bottom;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> Bookmark::GetSymbolNames() const
{
  auto symbolNames = GetCustomSymbolNames();
  if (symbolNames != nullptr)
    return symbolNames;

  symbolNames = make_unique_dp<SymbolNameZoomInfo>();

  symbolNames->insert(std::make_pair(1 /* zoomLevel */, "bookmark-default-xs"));
  symbolNames->insert(std::make_pair(8 /* zoomLevel */, "bookmark-default-s"));
  auto const iconType = GetBookmarkIconType(m_data.m_icon);
  symbolNames->insert(std::make_pair(14 /* zoomLevel */, "bookmark-" + iconType + "-m"));
  return symbolNames;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> Bookmark::GetCustomSymbolNames() const
{
  auto const it = m_data.m_properties.find(kCustomImageProperty);
  if (it == m_data.m_properties.end())
    return nullptr;

  auto symbolNames = make_unique_dp<SymbolNameZoomInfo>();
  strings::Tokenize(it->second, ";", [&](std::string_view token)
  {
    uint8_t zoomLevel = 1;
    auto pos = token.find(',');
    if (pos != std::string::npos && strings::to_uint(token.substr(0, pos), zoomLevel))
      token = token.substr(pos + 1);
    if (!token.empty() && zoomLevel >= 1 && zoomLevel <= scales::GetUpperStyleScale())
      symbolNames->emplace(zoomLevel, std::string(token));
  });

  if (symbolNames->empty())
    return nullptr;

  return symbolNames;
}

df::ColorConstant Bookmark::GetColorConstant() const
{
  switch (m_data.m_color.m_predefinedColor)
  {
  case kml::PredefinedColor::Red: return "BookmarkRed";
  case kml::PredefinedColor::Blue: return "BookmarkBlue";
  case kml::PredefinedColor::Purple: return "BookmarkPurple";
  case kml::PredefinedColor::Yellow: return "BookmarkYellow";
  case kml::PredefinedColor::Pink: return "BookmarkPink";
  case kml::PredefinedColor::Brown: return "BookmarkBrown";
  case kml::PredefinedColor::Green: return "BookmarkGreen";
  case kml::PredefinedColor::Orange: return "BookmarkOrange";
  case kml::PredefinedColor::DeepPurple: return "BookmarkDeepPurple";
  case kml::PredefinedColor::LightBlue: return "BookmarkLightBlue";
  case kml::PredefinedColor::Cyan: return "BookmarkCyan";
  case kml::PredefinedColor::Teal: return "BookmarkTeal";
  case kml::PredefinedColor::Lime: return "BookmarkLime";
  case kml::PredefinedColor::DeepOrange: return "BookmarkDeepOrange";
  case kml::PredefinedColor::Gray: return "BookmarkGray";
  case kml::PredefinedColor::BlueGray: return "BookmarkBlueGray";
  case kml::PredefinedColor::None:
  case kml::PredefinedColor::Count: return "BookmarkRed";
  }
  UNREACHABLE();
}

bool Bookmark::HasCreationAnimation() const
{
  return true;
}

kml::PredefinedColor Bookmark::GetColor() const
{
  return m_data.m_color.m_predefinedColor;
}

void Bookmark::SetColor(kml::PredefinedColor color)
{
  SetDirty();
  m_data.m_color.m_predefinedColor = color;
}

std::string Bookmark::GetPreferredName() const
{
  return GetPreferredBookmarkName(m_data);
}

kml::LocalizableString Bookmark::GetName() const
{
  return m_data.m_name;
}

void Bookmark::SetName(kml::LocalizableString const & name)
{
  SetDirty();
  m_data.m_name = name;
}

void Bookmark::SetName(std::string const & name, int8_t langCode)
{
  SetDirty();
  m_data.m_name[langCode] = name;
}

std::string Bookmark::GetCustomName() const
{
  return GetPreferredBookmarkStr(m_data.m_customName);
}

void Bookmark::SetCustomName(std::string const & customName)
{
  SetDirty();
  kml::SetDefaultStr(m_data.m_customName, customName);
}

m2::RectD Bookmark::GetViewport() const
{
  return m2::RectD(GetPivot(), GetPivot());
}

std::string Bookmark::GetDescription() const
{
  return GetPreferredBookmarkStr(m_data.m_description);
}

void Bookmark::SetDescription(std::string const & description)
{
  SetDirty();
  kml::SetDefaultStr(m_data.m_description, description);
}

kml::Timestamp Bookmark::GetTimeStamp() const
{
  return m_data.m_timestamp;
}

void Bookmark::SetTimeStamp(kml::Timestamp timeStamp)
{
  SetDirty();
  m_data.m_timestamp = timeStamp;
}

uint8_t Bookmark::GetScale() const
{
  return m_data.m_viewportScale;
}

void Bookmark::SetScale(uint8_t scale)
{
  SetDirty();
  m_data.m_viewportScale = scale;
}

kml::MarkGroupId Bookmark::GetGroupId() const
{
  return m_groupId;
}

bool Bookmark::CanFillPlacePageMetadata() const
{
  auto const & p = m_data.m_properties;
  if (auto const hours = p.find("hours"); hours != p.end() && !hours->second.empty())
    return true;
  return false;
}

void Bookmark::Attach(kml::MarkGroupId groupId)
{
  ASSERT_NOT_EQUAL(groupId, kml::kInvalidMarkGroupId, ());
  ASSERT_EQUAL(m_groupId, kml::kInvalidMarkGroupId, ());
  m_groupId = groupId;
}

void Bookmark::AttachCompilation(kml::MarkGroupId groupId)
{
  ASSERT(groupId != kml::kInvalidMarkGroupId, ());
  m_compilationIds.push_back(groupId);
}

void Bookmark::Detach()
{
  m_groupId = kml::kInvalidMarkGroupId;
  m_compilationIds.clear();
}

BookmarkCategory::BookmarkCategory(std::string const & name, kml::MarkGroupId groupId, bool autoSave)
  : Base(UserMark::Type::BOOKMARK)
  , m_autoSave(autoSave)
{
  m_data.m_id = groupId;
  SetName(name);
}

BookmarkCategory::BookmarkCategory(kml::CategoryData && data, bool autoSave)
  : Base(UserMark::Type::BOOKMARK)
  , m_autoSave(autoSave)
  , m_data(std::move(data))
{
  Base::SetIsVisible(m_data.m_visible);
}

void BookmarkCategory::SetIsVisible(bool isVisible)
{
  Base::SetIsVisible(isVisible);
  m_data.m_visible = isVisible;
}

void BookmarkCategory::SetName(std::string const & name)
{
  SetDirty(true /* updateModificationTime */);
  kml::SetDefaultStr(m_data.m_name, name);
}

void BookmarkCategory::SetDescription(std::string const & desc)
{
  SetDirty(true /* updateModificationTime */);
  kml::SetDefaultStr(m_data.m_description, desc);
}

void BookmarkCategory::SetServerId(std::string const & serverId)
{
  if (m_serverId == serverId)
    return;

  SetDirty(true /* updateModificationTime */);
  m_serverId = serverId;
}

void BookmarkCategory::SetTags(std::vector<std::string> const & tags)
{
  if (m_data.m_tags == tags)
    return;

  SetDirty(true /* updateModificationTime */);
  m_data.m_tags = tags;
}

void BookmarkCategory::SetCustomProperty(std::string const & key, std::string const & value)
{
  auto it = m_data.m_properties.find(key);
  if (it != m_data.m_properties.end() && it->second == value)
    return;

  SetDirty(true /* updateModificationTime */);
  m_data.m_properties[key] = value;
}

std::string BookmarkCategory::GetName() const
{
  return GetPreferredBookmarkStr(m_data.m_name);
}

bool BookmarkCategory::HasElevationProfile() const
{
  auto const it = m_data.m_properties.find(kHasElevationProfileProperty);
  return (it != m_data.m_properties.end()) && (it->second != "0");
}

void BookmarkCategory::SetAuthor(std::string const & name, std::string const & id)
{
  if (m_data.m_authorName == name && m_data.m_authorId == id)
    return;

  SetDirty(true /* updateModificationTime */);
  m_data.m_authorName = name;
  m_data.m_authorId = id;
}

void BookmarkCategory::SetAccessRules(kml::AccessRules accessRules)
{
  if (m_data.m_accessRules == accessRules)
    return;

  SetDirty(true /* updateModificationTime */);
  m_data.m_accessRules = accessRules;
}

// static
kml::PredefinedColor BookmarkCategory::GetDefaultColor()
{
  return kml::PredefinedColor::Red;
}

void BookmarkCategory::SetDirty(bool updateModificationDate)
{
  Base::SetDirty(updateModificationDate);
  if (updateModificationDate)
    m_data.m_lastModified = kml::TimestampClock::now();
}
