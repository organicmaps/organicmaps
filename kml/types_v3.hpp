#pragma once

#include "kml/types.hpp"

#include "base/visitor.hpp"

#include <cmath>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// V3 kml structures are used to deserialize data of lower versions (V0 - V3).
namespace kml
{
struct BookmarkDataV3
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(BookmarkDataV3, visitor(m_id, "id"), visitor(m_name, "name"),
                                  visitor(m_description, "description"), visitor(m_featureTypes, "featureTypes"),
                                  visitor(m_customName, "customName"), visitor(m_color, "color"),
                                  visitor(m_icon, "icon"), visitor(m_viewportScale, "viewportScale"),
                                  visitor(m_timestamp, "timestamp"), visitor(m_point, "point"),
                                  visitor(m_boundTracks, "boundTracks"), VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_customName)

  bool operator==(BookmarkDataV3 const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_name == data.m_name && m_description == data.m_description &&
           m_color == data.m_color && m_icon == data.m_icon && m_viewportScale == data.m_viewportScale &&
           IsEqual(m_timestamp, data.m_timestamp) && m_point.EqualDxDy(data.m_point, kEps) &&
           m_featureTypes == data.m_featureTypes && m_customName == data.m_customName &&
           m_boundTracks == data.m_boundTracks;
  }

  bool operator!=(BookmarkDataV3 const & data) const { return !operator==(data); }

  BookmarkData ConvertToLatestVersion()
  {
    BookmarkData data;
    data.m_id = m_id;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_featureTypes = m_featureTypes;
    data.m_customName = m_customName;
    data.m_color = m_color;
    data.m_icon = m_icon;
    data.m_viewportScale = m_viewportScale;
    data.m_timestamp = m_timestamp;
    data.m_point = m_point;
    data.m_boundTracks = m_boundTracks;
    return data;
  }

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

struct TrackDataV3
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackDataV3, visitor(m_id, "id"), visitor(m_localId, "localId"),
                                  visitor(m_name, "name"), visitor(m_description, "description"),
                                  visitor(m_layers, "layers"), visitor(m_timestamp, "timestamp"),
                                  visitor(m_points, "points"), VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description)

  bool operator==(TrackDataV3 const & data) const
  {
    return m_id == data.m_id && m_localId == data.m_localId && m_name == data.m_name &&
           m_description == data.m_description && m_layers == data.m_layers && IsEqual(m_timestamp, data.m_timestamp) &&
           IsEqual(m_points, data.m_points);
  }

  bool operator!=(TrackDataV3 const & data) const { return !operator==(data); }

  TrackData ConvertToLatestVersion()
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry.FromPoints(m_points);
    return data;
  }

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

struct CategoryDataV3
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(CategoryDataV3, visitor(m_id, "id"), visitor(m_name, "name"),
                                  visitor(m_imageUrl, "imageUrl"), visitor(m_annotation, "annotation"),
                                  visitor(m_description, "description"), visitor(m_visible, "visible"),
                                  visitor(m_authorName, "authorName"), visitor(m_authorId, "authorId"),
                                  visitor(m_rating, "rating"), visitor(m_reviewsNumber, "reviewsNumber"),
                                  visitor(m_lastModified, "lastModified"), visitor(m_accessRules, "accessRules"),
                                  visitor(m_tags, "tags"), visitor(m_cities, "cities"),
                                  visitor(m_languageCodes, "languageCodes"), visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_annotation, m_description, m_imageUrl, m_authorName, m_authorId,
                      m_tags, m_properties)

  bool operator==(CategoryDataV3 const & data) const
  {
    double constexpr kEps = 1e-5;
    return m_id == data.m_id && m_name == data.m_name && m_imageUrl == data.m_imageUrl &&
           m_annotation == data.m_annotation && m_description == data.m_description && m_visible == data.m_visible &&
           m_accessRules == data.m_accessRules && m_authorName == data.m_authorName && m_authorId == data.m_authorId &&
           fabs(m_rating - data.m_rating) < kEps && m_reviewsNumber == data.m_reviewsNumber &&
           IsEqual(m_lastModified, data.m_lastModified) && m_tags == data.m_tags && IsEqual(m_cities, data.m_cities) &&
           m_languageCodes == data.m_languageCodes && m_properties == data.m_properties;
  }

  bool operator!=(CategoryDataV3 const & data) const { return !operator==(data); }

  CategoryData ConvertToLatestVersion()
  {
    // We don't convert m_cities to m_toponyms, since m_cities have been never used.
    CategoryData data;
    data.m_id = m_id;
    data.m_name = m_name;
    data.m_imageUrl = m_imageUrl;
    data.m_annotation = m_annotation;
    data.m_description = m_description;
    data.m_visible = m_visible;
    data.m_authorName = m_authorName;
    data.m_authorId = m_authorId;
    data.m_lastModified = m_lastModified;
    data.m_rating = m_rating;
    data.m_reviewsNumber = m_reviewsNumber;
    data.m_accessRules = m_accessRules;
    data.m_tags = m_tags;
    data.m_languageCodes = m_languageCodes;
    data.m_properties = m_properties;
    return data;
  }

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

struct FileDataV3
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV3, visitor(m_serverId, "serverId"), visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"), visitor(m_tracksData, "tracks"))

  bool operator==(FileDataV3 const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileDataV3 const & data) const { return !operator==(data); }

  // Device id (unused, it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV3 m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV3> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV3> m_tracksData;

  FileData ConvertToLatestVersion()
  {
    FileData data;
    data.m_deviceId = m_deviceId;
    data.m_serverId = m_serverId;
    data.m_categoryData = m_categoryData.ConvertToLatestVersion();

    data.m_bookmarksData.reserve(m_bookmarksData.size());
    for (auto & d : m_bookmarksData)
      data.m_bookmarksData.emplace_back(d.ConvertToLatestVersion());

    data.m_tracksData.reserve(m_tracksData.size());
    for (auto & d : m_tracksData)
      data.m_tracksData.emplace_back(d.ConvertToLatestVersion());

    return data;
  }
};
}  // namespace kml
