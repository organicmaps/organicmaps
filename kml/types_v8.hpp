#pragma once

#include "kml/types_v9.hpp"

namespace kml
{

struct BookmarkDataV8
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(BookmarkDataV8, visitor(m_id, "id"), visitor(m_name, "name"),
                                  visitor(m_description, "description"), visitor(m_featureTypes, "featureTypes"),
                                  visitor(m_customName, "customName"), visitor(m_color, "color"),
                                  visitor(m_icon, "icon"), visitor(m_viewportScale, "viewportScale"),
                                  visitor(m_timestamp, "timestamp"), visitor(m_point, "point"),
                                  visitor(m_boundTracks, "boundTracks"), visitor(m_visible, "visible"),
                                  visitor(m_nearestToponym, "nearestToponym"), visitor(m_properties, "properties"),
                                  visitor(m_compilations, "compilations"), VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_customName, m_nearestToponym, m_properties)

  bool operator==(BookmarkDataV8 const & data) const
  {
    return m_id == data.m_id && m_name == data.m_name && m_description == data.m_description &&
           m_color == data.m_color && m_icon == data.m_icon && m_viewportScale == data.m_viewportScale &&
           IsEqual(m_timestamp, data.m_timestamp) && m_point.EqualDxDy(data.m_point, kMwmPointAccuracy) &&
           m_featureTypes == data.m_featureTypes && m_customName == data.m_customName &&
           m_boundTracks == data.m_boundTracks && m_visible == data.m_visible &&
           m_nearestToponym == data.m_nearestToponym && m_properties == data.m_properties &&
           m_compilations == data.m_compilations;
  }

  bool operator!=(BookmarkDataV8 const & data) const { return !operator==(data); }

  BookmarkDataV8() = default;

  // Initialize using latest version of BookmarkData
  BookmarkDataV8(BookmarkData const & src)
  {
    // Copy all fields from `src` except `m_minZoom`
    m_id = src.m_id;
    m_name = src.m_name;
    m_description = src.m_description;
    m_featureTypes = src.m_featureTypes;
    m_customName = src.m_customName;
    m_color = src.m_color;
    m_icon = src.m_icon;
    m_viewportScale = src.m_viewportScale;
    m_timestamp = src.m_timestamp;
    m_point = src.m_point;
    m_boundTracks = src.m_boundTracks;
    m_visible = src.m_visible;
    m_nearestToponym = src.m_nearestToponym;
    m_properties = src.m_properties;
    m_compilations = src.m_compilations;
    m_collectionIndex = src.m_collectionIndex;
  }

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
    data.m_visible = m_visible;
    data.m_nearestToponym = m_nearestToponym;
    data.m_properties = m_properties;
    data.m_compilations = m_compilations;
    return data;
  }

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
  // Key-value properties.
  Properties m_properties;
  // List of compilationIds.
  std::vector<CompilationId> m_compilations;
};

using TrackDataV8 = TrackDataV9;
using CategoryDataV8 = CategoryDataV9;

struct FileDataV8
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV8, visitor(m_serverId, "serverId"), visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"), visitor(m_tracksData, "tracks"),
                                  visitor(m_compilationData, "compilations"))

  bool operator==(FileDataV8 const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData &&
           m_compilationData == data.m_compilationData;
  }

  bool operator!=(FileDataV8 const & data) const { return !operator==(data); }

  FileData ConvertToLatestVersion()
  {
    FileData data;
    data.m_deviceId = m_deviceId;
    data.m_serverId = m_serverId;

    data.m_categoryData = m_categoryData;

    data.m_bookmarksData.reserve(m_bookmarksData.size());
    for (auto & d : m_bookmarksData)
      data.m_bookmarksData.emplace_back(d.ConvertToLatestVersion());

    data.m_tracksData = m_tracksData;

    return data;
  }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV8 m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV8> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV8> m_tracksData;
  // Compilation collection.
  std::vector<CategoryDataV8> m_compilationData;
};
}  // namespace kml
