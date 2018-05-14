#include "map/bookmark.hpp"
#include "map/bookmark_helpers.hpp"

namespace
{
std::string GetBookmarkIconType(kml::BookmarkIcon const & icon)
{
  switch (icon)
  {
  case kml::BookmarkIcon::None: return "default";
  case kml::BookmarkIcon::Count:
    ASSERT(false, ("Invalid bookmark icon type"));
    return {};
  }
}
}  // namespace

Bookmark::Bookmark(m2::PointD const & ptOrg)
  : Base(ptOrg, UserMark::BOOKMARK)
  , m_groupId(kml::kInvalidMarkGroupId)
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

dp::Anchor Bookmark::GetAnchor() const
{
  return dp::Bottom;
}

drape_ptr<df::UserPointMark::SymbolNameZoomInfo> Bookmark::GetSymbolNames() const
{
  auto symbol = make_unique_dp<SymbolNameZoomInfo>();
  symbol->insert(std::make_pair(1 /* zoomLevel */, "bookmark-default-xs"));
  symbol->insert(std::make_pair(8 /* zoomLevel */, "bookmark-default-s"));
  auto const iconType = GetBookmarkIconType(m_data.m_icon);
  symbol->insert(std::make_pair(14 /* zoomLevel */, "bookmark-" + iconType + "-m"));
  return symbol;
}

df::ColorConstant Bookmark::GetColorConstant() const
{
  switch (m_data.m_color.m_predefinedColor)
  {
    case kml::PredefinedColor::Red:
      return "BookmarkRed";
    case kml::PredefinedColor::Blue:
      return "BookmarkBlue";
    case kml::PredefinedColor::Purple:
      return "BookmarkPurple";
    case kml::PredefinedColor::Yellow:
      return "BookmarkYellow";
    case kml::PredefinedColor::Pink:
      return "BookmarkPink";
    case kml::PredefinedColor::Brown:
      return "BookmarkBrown";
    case kml::PredefinedColor::Green:
      return "BookmarkGreen";
    case kml::PredefinedColor::Orange:
      return "BookmarkOrange";
    case kml::PredefinedColor::None:
    case kml::PredefinedColor::Count:
      return "BookmarkRed";
  }
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
  return kml::GetDefaultStr(m_data.m_customName);
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
  return kml::GetDefaultStr(m_data.m_description);
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

void Bookmark::Attach(kml::MarkGroupId groupId)
{
  ASSERT(m_groupId == kml::kInvalidMarkGroupId, ());
  m_groupId = groupId;
}

void Bookmark::Detach()
{
  m_groupId = kml::kInvalidMarkGroupId;
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

BookmarkCategory::~BookmarkCategory()
{
}

void BookmarkCategory::SetIsVisible(bool isVisible)
{
  Base::SetIsVisible(isVisible);
  m_data.m_visible = isVisible;
}

void BookmarkCategory::SetName(std::string const & name)
{
  SetDirty();
  kml::SetDefaultStr(m_data.m_name, name);
}

std::string BookmarkCategory::GetName() const
{
  return kml::GetDefaultStr(m_data.m_name);
}

// static
kml::PredefinedColor BookmarkCategory::GetDefaultColor()
{
  return kml::PredefinedColor::Red;
}

void BookmarkCategory::SetDirty()
{
  Base::SetDirty();
  m_data.m_lastModified = std::chrono::system_clock::now();
}
