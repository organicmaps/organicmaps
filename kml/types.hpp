#pragma once

#include "kml/type_utils.hpp"

#include "base/visitor.hpp"

#include <cmath>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace kml
{
enum class PredefinedColor : uint8_t
{
  // Do not change the order because of binary serialization.
  None = 0,
  Red,
  Blue,
  Purple,
  Yellow,
  Pink,
  Brown,
  Green,
  Orange,

  Count
};

inline std::string DebugPrint(PredefinedColor color)
{
  switch (color)
  {
  case PredefinedColor::None: return "None";
  case PredefinedColor::Red: return "Red";
  case PredefinedColor::Blue: return "Blue";
  case PredefinedColor::Purple: return "Purple";
  case PredefinedColor::Yellow: return "Yellow";
  case PredefinedColor::Pink: return "Pink";
  case PredefinedColor::Brown: return "Brown";
  case PredefinedColor::Green: return "Green";
  case PredefinedColor::Orange: return "Orange";
  case PredefinedColor::Count: return {};
  }
  UNREACHABLE();
}

enum class AccessRules : uint8_t
{
  // Do not change the order because of binary serialization.
  Local = 0,
  Public,
  DirectLink,
  P2P,
  Paid,

  Count
};

inline std::string DebugPrint(AccessRules accessRules)
{
  switch (accessRules)
  {
  case AccessRules::Local: return "Local";
  case AccessRules::DirectLink: return "DirectLink";
  case AccessRules::P2P: return "P2P";
  case AccessRules::Paid: return "Paid";
  case AccessRules::Public: return "Public";
  case AccessRules::Count: return {};
  }
  UNREACHABLE();
}

enum class BookmarkIcon : uint16_t
{
  // Do not change the order because of binary serialization.
  None = 0,
  Hotel,
  Animals,
  Buddhism,
  Building,
  Christianity,
  Entertainment,
  Exchange,
  Food,
  Gas,
  Judaism,
  Medicine,
  Mountain,
  Museum,
  Islam,
  Park,
  Parking,
  Shop,
  Sights,
  Swim,
  Water,
  Count
};

inline std::string DebugPrint(BookmarkIcon icon)
{
  switch (icon)
  {
  case BookmarkIcon::None: return "None";
  case BookmarkIcon::Hotel: return "Hotel";
  case BookmarkIcon::Animals: return "Animals";
  case BookmarkIcon::Buddhism: return "Buddhism";
  case BookmarkIcon::Building: return "Building";
  case BookmarkIcon::Christianity: return "Christianity";
  case BookmarkIcon::Entertainment: return "Entertainment";
  case BookmarkIcon::Exchange: return "Exchange";
  case BookmarkIcon::Food: return "Food";
  case BookmarkIcon::Gas: return "Gas";
  case BookmarkIcon::Judaism: return "Judaism";
  case BookmarkIcon::Medicine: return "Medicine";
  case BookmarkIcon::Mountain: return "Mountain";
  case BookmarkIcon::Museum: return "Museum";
  case BookmarkIcon::Islam: return "Islam";
  case BookmarkIcon::Park: return "Park";
  case BookmarkIcon::Parking: return "Parking";
  case BookmarkIcon::Shop: return "Shop";
  case BookmarkIcon::Sights: return "Sights";
  case BookmarkIcon::Swim: return "Swim";
  case BookmarkIcon::Water: return "Water";
  case BookmarkIcon::Count: return {};
  }
  UNREACHABLE();
}

struct ColorData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(ColorData, visitor(m_predefinedColor, "predefinedColor"),
                                  visitor(m_rgba, "rgba"))

  bool operator==(ColorData const & data) const
  {
    return m_predefinedColor == data.m_predefinedColor && m_rgba == data.m_rgba;
  }

  bool operator!=(ColorData const & data) const { return !operator==(data); }

  // Predefined color.
  PredefinedColor m_predefinedColor = PredefinedColor::None;
  // Color in RGBA format.
  uint32_t m_rgba = 0;
};

struct BookmarkData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(BookmarkData, visitor(m_id, "id"),
                                  visitor(m_name, "name"),
                                  visitor(m_description, "description"),
                                  visitor(m_featureTypes, "featureTypes"),
                                  visitor(m_customName, "customName"),
                                  visitor(m_color, "color"),
                                  visitor(m_icon, "icon"),
                                  visitor(m_viewportScale, "viewportScale"),
                                  visitor(m_timestamp, "timestamp"),
                                  visitor(m_point, "point"),
                                  visitor(m_boundTracks, "boundTracks"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_customName)

  bool operator==(BookmarkData const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_name == data.m_name &&
           m_description == data.m_description &&
           m_color == data.m_color && m_icon == data.m_icon &&
           m_viewportScale == data.m_viewportScale &&
           IsEqual(m_timestamp, data.m_timestamp) &&
           m_point.EqualDxDy(data.m_point, kEps) &&
           m_featureTypes == data.m_featureTypes &&
           m_customName == data.m_customName &&
           m_boundTracks == data.m_boundTracks;
  }

  bool operator!=(BookmarkData const & data) const { return !operator==(data); }

  // Unique id (it will not be serialized in text files).
  MarkId m_id = kInvalidMarkId;
  // Bookmark's name.
  LocalizableString m_name;
  // Bookmark's description.
  LocalizableString m_description;
  // Bound feature's types.
  std::vector<uint32_t> m_featureTypes;
  // Custom bookmark's name.
  LocalizableString m_customName;
  // Bookmark's color.
  ColorData m_color;
  // Bookmark's icon.
  BookmarkIcon m_icon = BookmarkIcon::None;
  // Viewport scale. 0 is a default value (no scale set).
  uint8_t m_viewportScale = 0;
  // Creation timestamp.
  Timestamp m_timestamp = {};
  // Coordinates in mercator.
  m2::PointD m_point;
  // Bound tracks (vector contains local track ids).
  std::vector<LocalId> m_boundTracks;
};

struct TrackLayer
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackLayer, visitor(m_lineWidth, "lineWidth"),
                                  visitor(m_color, "color"))

  bool operator==(TrackLayer const & layer) const
  {
    double constexpr kEps = 1e-5;
    return m_color == layer.m_color && fabs(m_lineWidth - layer.m_lineWidth) < kEps;
  }

  bool operator!=(TrackLayer const & layer) const { return !operator==(layer); }

  // Line width in pixels. Valid range is [kMinLineWidth; kMaxLineWidth].
  double m_lineWidth = 5.0;
  // Layer's color.
  ColorData m_color;
};

struct TrackData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackData, visitor(m_id, "id"),
                                  visitor(m_localId, "localId"),
                                  visitor(m_name, "name"),
                                  visitor(m_description, "description"),
                                  visitor(m_layers, "layers"),
                                  visitor(m_timestamp, "timestamp"),
                                  visitor(m_points, "points"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description)

  bool operator==(TrackData const & data) const
  {
    return m_id == data.m_id && m_localId == data.m_localId && m_name == data.m_name &&
           m_description == data.m_description && m_layers == data.m_layers &&
           IsEqual(m_timestamp, data.m_timestamp) && IsEqual(m_points, data.m_points);
  }

  bool operator!=(TrackData const & data) const { return !operator==(data); }
  
  // Unique id (it will not be serialized in text files).
  TrackId m_id = kInvalidTrackId;
  // Local track id.
  LocalId m_localId = 0;
  // Track's name.
  LocalizableString m_name;
  // Track's description.
  LocalizableString m_description;
  // Layers.
  std::vector<TrackLayer> m_layers;
  // Creation timestamp.
  Timestamp m_timestamp = {};
  // Points.
  std::vector<m2::PointD> m_points;
};

struct CategoryData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(CategoryData, visitor(m_id, "id"),
                                  visitor(m_name, "name"),
                                  visitor(m_imageUrl, "imageUrl"),
                                  visitor(m_annotation, "annotation"),
                                  visitor(m_description, "description"),
                                  visitor(m_visible, "visible"),
                                  visitor(m_authorName, "authorName"),
                                  visitor(m_authorId, "authorId"),
                                  visitor(m_rating, "rating"),
                                  visitor(m_reviewsNumber, "reviewsNumber"),
                                  visitor(m_lastModified, "lastModified"),
                                  visitor(m_accessRules, "accessRules"),
                                  visitor(m_tags, "tags"),
                                  visitor(m_cities, "cities"),
                                  visitor(m_languageCodes, "languageCodes"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_annotation, m_description,
                      m_imageUrl, m_authorName, m_authorId, m_tags, m_properties)

  bool operator==(CategoryData const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_name == data.m_name && m_imageUrl == data.m_imageUrl &&
           m_annotation == data.m_annotation && m_description == data.m_description &&
           m_visible == data.m_visible && m_accessRules == data.m_accessRules &&
           m_authorName == data.m_authorName && m_authorId == data.m_authorId &&
           fabs(m_rating - data.m_rating) < kEps && m_reviewsNumber == data.m_reviewsNumber &&
           IsEqual(m_lastModified, data.m_lastModified) && m_tags == data.m_tags &&
           IsEqual(m_cities, data.m_cities) && m_languageCodes == data.m_languageCodes &&
           m_properties == data.m_properties;
  }

  bool operator!=(CategoryData const & data) const { return !operator==(data); }

  // Unique id (it will not be serialized in text files).
  MarkGroupId m_id = kInvalidMarkGroupId;
  // Category's name.
  LocalizableString m_name;
  // Image URL.
  std::string m_imageUrl;
  // Category's description.
  LocalizableString m_annotation;
  // Category's description.
  LocalizableString m_description;
  // Collection visibility.
  bool m_visible = true;
  // Author's name.
  std::string m_authorName;
  // Author's id.
  std::string m_authorId;
  // Last modification timestamp.
  Timestamp m_lastModified;
  // Rating.
  double m_rating = 0.0;
  // Number of reviews.
  uint32_t m_reviewsNumber = 0;
  // Access rules.
  AccessRules m_accessRules = AccessRules::Local;
  // Collection of tags.
  std::vector<std::string> m_tags;
  // Collection of cities coordinates.
  std::vector<m2::PointD> m_cities;
  // Language codes.
  std::vector<int8_t> m_languageCodes;
  // Key-value properties.
  Properties m_properties;
};

struct FileData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileData, visitor(m_serverId, "serverId"),
                                  visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"),
                                  visitor(m_tracksData, "tracks"))

  bool operator==(FileData const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileData const & data) const { return !operator==(data); }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryData m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkData> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackData> m_tracksData;
};
}  // namespace kml
