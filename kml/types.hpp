#pragma once

#include "kml/type_utils.hpp"

#include "coding/point_coding.hpp"

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

  // Extended colors.
  DeepPurple,
  LightBlue,
  Cyan,
  Teal,
  Lime,
  DeepOrange,
  Gray,
  BlueGray,

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
  case PredefinedColor::DeepPurple: return "DeepPurple";
  case PredefinedColor::LightBlue: return "LightBlue";
  case PredefinedColor::Cyan: return "Cyan";
  case PredefinedColor::Teal: return "Teal";
  case PredefinedColor::Lime: return "Lime";
  case PredefinedColor::DeepOrange: return "DeepOrange";
  case PredefinedColor::Gray: return "Gray";
  case PredefinedColor::BlueGray: return "BlueGray";
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
  AuthorOnly,

  Count
};

inline std::string DebugPrint(AccessRules accessRules)
{
  switch (accessRules)
  {
  case AccessRules::Local: return "Local";
  case AccessRules::Public: return "Public";
  case AccessRules::DirectLink: return "DirectLink";
  case AccessRules::P2P: return "P2P";
  case AccessRules::Paid: return "Paid";
  case AccessRules::AuthorOnly: return "AuthorOnly";
  case AccessRules::Count: return {};
  }
  UNREACHABLE();
}

enum class CompilationType : uint8_t
{
  // Do not change the order because of binary serialization.
  Category = 0,
  Collection,
  Day,

  Count
};

inline std::string DebugPrint(CompilationType compilationType)
{
  switch (compilationType)
  {
  case CompilationType::Category: return "Category";
  case CompilationType::Collection: return "Collection";
  case CompilationType::Day: return "Day";
  case CompilationType::Count: return {};
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

  // Extended icons.
  Bar,
  Transport,
  Viewpoint,
  Sport,
  Start,
  Finish,

  Count
};

inline std::string ToString(BookmarkIcon icon)
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
  case BookmarkIcon::Bar: return "Bar";
  case BookmarkIcon::Transport: return "Transport";
  case BookmarkIcon::Viewpoint: return "Viewpoint";
  case BookmarkIcon::Sport: return "Sport";
  case BookmarkIcon::Start: return "Start";
  case BookmarkIcon::Finish: return "Finish";
  case BookmarkIcon::Count: return {};
  }
  UNREACHABLE();
}

// Note: any changes in binary format of this structure
// will affect previous kmb versions, where this structure was used.
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

// This structure is used in FileDataV6 because
// its binary format is the same as in kmb version 6.
// Make a copy of this structure in a file types_v<n>.hpp
// in case of any changes of its binary format.
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
                                  visitor(m_visible, "visible"),
                                  visitor(m_nearestToponym, "nearestToponym"),
                                  visitor(m_minZoom, "minZoom"),
                                  visitor(m_compilations, "compilations"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_customName,
                      m_nearestToponym, m_properties)

  bool operator==(BookmarkData const & data) const
  {
    return m_id == data.m_id && m_name == data.m_name &&
           m_description == data.m_description &&
           m_color == data.m_color && m_icon == data.m_icon &&
           m_viewportScale == data.m_viewportScale &&
           IsEqual(m_timestamp, data.m_timestamp) &&
           m_point.EqualDxDy(data.m_point, kMwmPointAccuracy) &&
           m_featureTypes == data.m_featureTypes &&
           m_customName == data.m_customName &&
           m_boundTracks == data.m_boundTracks &&
           m_visible == data.m_visible &&
           m_nearestToponym == data.m_nearestToponym &&
           m_minZoom == data.m_minZoom &&
           m_compilations == data.m_compilations &&
           m_properties == data.m_properties;
  }

  bool operator!=(BookmarkData const & data) const { return !operator==(data); }

  // Unique id (it will not be serialized in text files).
  MarkId m_id = kInvalidMarkId;
  // Bookmark's name.
  LocalizableString m_name;
  // Bookmark's description.
  LocalizableString m_description;
  // Bound feature's types: type indices sorted by importance, the most
  // important one goes first.
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
  // Visibility.
  bool m_visible = true;
  // Nearest toponym.
  std::string m_nearestToponym;
  // Minimal zoom when bookmark is visible.
  int m_minZoom = 1;
  // List of compilationIds.
  std::vector<CompilationId> m_compilations;
  // Key-value properties.
  Properties m_properties;
};

// Note: any changes in binary format of this structure
// will affect previous kmb versions, where this structure was used.
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
                                  visitor(m_pointsWithAltitudes, "pointsWithAltitudes"),
                                  visitor(m_visible, "visible"),
                                  visitor(m_nearestToponyms, "nearestToponyms"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  bool operator==(TrackData const & data) const
  {
    return m_id == data.m_id && m_localId == data.m_localId && m_name == data.m_name &&
           m_description == data.m_description && m_layers == data.m_layers &&
           IsEqual(m_timestamp, data.m_timestamp) &&
           IsEqual(m_pointsWithAltitudes, data.m_pointsWithAltitudes) &&
           m_visible == data.m_visible && m_nearestToponyms == data.m_nearestToponyms &&
           m_properties == data.m_properties;
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
  // Points with altitudes.
  std::vector<geometry::PointWithAltitude> m_pointsWithAltitudes;
  // Visibility.
  bool m_visible = true;
  // Nearest toponyms.
  std::vector<std::string> m_nearestToponyms;
  // Key-value properties.
  Properties m_properties;
};

// This structure is used in FileDataV6 because
// its binary format is the same as in kmb version 6.
// Make a copy of this structure in a file types_v<n>.hpp
// in case of any changes of its binary format.
struct CategoryData
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(CategoryData, visitor(m_id, "id"),
                                  visitor(m_compilationId, "compilationId"),
                                  visitor(m_type, "type"),
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
                                  visitor(m_toponyms, "toponyms"),
                                  visitor(m_languageCodes, "languageCodes"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_annotation, m_description,
                      m_imageUrl, m_authorName, m_authorId, m_tags, m_toponyms, m_properties)

  bool operator==(CategoryData const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_compilationId == data.m_compilationId &&
           m_type == data.m_type && m_name == data.m_name && m_imageUrl == data.m_imageUrl &&
           m_annotation == data.m_annotation && m_description == data.m_description &&
           m_visible == data.m_visible && m_accessRules == data.m_accessRules &&
           m_authorName == data.m_authorName && m_authorId == data.m_authorId &&
           fabs(m_rating - data.m_rating) < kEps && m_reviewsNumber == data.m_reviewsNumber &&
           IsEqual(m_lastModified, data.m_lastModified) && m_tags == data.m_tags &&
           m_toponyms == data.m_toponyms && m_languageCodes == data.m_languageCodes &&
           m_properties == data.m_properties;
  }

  bool operator!=(CategoryData const & data) const { return !operator==(data); }

  // Unique id (it will not be serialized in text files).
  MarkGroupId m_id = kInvalidMarkGroupId;
  // Id unique within single kml (have to be serialized in text files).
  CompilationId m_compilationId = kInvalidCompilationId;
  // Unique ids of nested groups (it will not be serialized in text files).
  GroupIdCollection m_compilationIds;
  // Compilation's type
  CompilationType m_type = CompilationType::Category;
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
  // Collection of geo ids for relevant toponyms.
  std::vector<std::string> m_toponyms;
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
                                  visitor(m_tracksData, "tracks"),
                                  visitor(m_compilationsData, "compilations"))

  bool operator==(FileData const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData &&
           m_compilationsData == data.m_compilationsData;
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
  // Compilation collection.
  std::vector<CategoryData> m_compilationsData;
};

void SetBookmarksMinZoom(FileData & fileData, double countPerTile, int maxZoom);
  
inline std::string DebugPrint(BookmarkIcon icon)
{
  return ToString(icon);
}
}  // namespace kml
