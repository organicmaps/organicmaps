#include "map/bookmark.hpp"
#include "map/api_mark_point.hpp"
#include "map/bookmark_manager.hpp"
#include "map/track.hpp"

#include "kml/serdes.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/hex.hpp"
#include "coding/parse_xml.hpp"  // LoadFromKML
#include "coding/internal/file_data.hpp"

#include "drape/drape_global.hpp"
#include "drape/color.hpp"

#include "drape_frontend/color_constants.hpp"

#include "platform/platform.hpp"

#include "base/scope_guard.hpp"
#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <kml/serdes_binary.hpp>

Bookmark::Bookmark(m2::PointD const & ptOrg)
  : Base(ptOrg, UserMark::BOOKMARK)
  , m_groupId(df::kInvalidMarkGroupId)
{
  m_data.m_point = ptOrg;
}

Bookmark::Bookmark(kml::BookmarkData const & data)
  : Base(data.m_point, UserMark::BOOKMARK)
  , m_data(data)
  , m_groupId(df::kInvalidMarkGroupId)
{}

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
  symbol->insert(std::make_pair(11 /* zoomLevel */, "bookmark-default-m"));
  symbol->insert(std::make_pair(15 /* zoomLevel */, "bookmark-default-l"));
  return symbol;
}

df::ColorConstant Bookmark::GetColor() const
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

std::string Bookmark::GetIcon() const
{
  switch (m_data.m_color.m_predefinedColor)
  {
    case kml::PredefinedColor::Red:
      return "placemark-red";
    case kml::PredefinedColor::Blue:
      return "placemark-blue";
    case kml::PredefinedColor::Purple:
      return "placemark-purple";
    case kml::PredefinedColor::Yellow:
      return "placemark-yellow";
    case kml::PredefinedColor::Pink:
      return "placemark-pink";
    case kml::PredefinedColor::Brown:
      return "placemark-brown";
    case kml::PredefinedColor::Green:
      return "placemark-green";
    case kml::PredefinedColor::Orange:
      return "placemark-orange";
    case kml::PredefinedColor::None:
    case kml::PredefinedColor::Count:
      return "placemark-red";
  }
}

bool Bookmark::HasCreationAnimation() const
{
  return true;
}

std::string Bookmark::GetName() const
{
  return kml::GetDefaultStr(m_data.m_name);
}

void Bookmark::SetName(std::string const & name)
{
  SetDirty();
  kml::SetDefaultStr(m_data.m_name, name);
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
  m_data.m_timestamp = timeStamp;
}

uint8_t Bookmark::GetScale() const
{
  return m_data.m_viewportScale;
}

void Bookmark::SetScale(uint8_t scale)
{
  m_data.m_viewportScale = scale;
}

df::MarkGroupID Bookmark::GetGroupId() const
{
  return m_groupId;
}

void Bookmark::Attach(df::MarkGroupID groupId)
{
  ASSERT(m_groupId == df::kInvalidMarkGroupId, ());
  m_groupId = groupId;
}

void Bookmark::Detach()
{
  m_groupId = df::kInvalidMarkGroupId;
}

BookmarkCategory::BookmarkCategory(std::string const & name, df::MarkGroupID groupId, bool autoSave)
  : Base(UserMark::Type::BOOKMARK)
  , m_groupId(groupId)
  , m_autoSave(autoSave)
{
  SetName(name);
}

BookmarkCategory::BookmarkCategory(kml::CategoryData const & data, df::MarkGroupID groupId, bool autoSave)
  : Base(UserMark::Type::BOOKMARK)
  , m_groupId(groupId)
  , m_autoSave(autoSave)
  , m_data(data)
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

std::string BookmarkCategory::GetDefaultType()
{
  return style::GetDefaultStyle();
}

std::unique_ptr<kml::FileData> LoadKMLFile(std::string const & file, bool useBinary)
{
  try
  {
    return LoadKMLData(FileReader(file), useBinary);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("Error while loading bookmarks from", file, e.what()));
  }
  return nullptr;
}

std::unique_ptr<kml::FileData> LoadKMLData(Reader const & reader, bool useBinary)
{
  auto data = std::make_unique<kml::FileData>();
  try
  {
    if (useBinary)
    {
      kml::binary::DeserializerKml des(*data.get());
      des.Deserialize(reader);
    }
    else
    {
      kml::DeserializerKml des(*data.get());
      des.Deserialize(reader);
    }
  }
  catch (FileReader::Exception const & exc)
  {
    LOG(LWARNING, ("KML ", useBinary ? "binary" : "text", " deserialization failure: ", exc.what()));
    data.reset();
  }
  return data;
}
