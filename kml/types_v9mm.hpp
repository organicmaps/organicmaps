#pragma once

#include "kml/types.hpp"
#include "kml/types_v8mm.hpp"

namespace kml
{

MultiGeometry mergeGeometry(std::vector<MultiGeometry> aGeometries);

struct TrackDataV9MM : TrackDataV8MM
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(TrackDataV9MM, visitor(m_id, "id"),
                                  visitor(m_localId, "localId"),
                                  visitor(m_name, "name"),
                                  visitor(m_description, "description"),
                                  visitor(m_layers, "layers"),
                                  visitor(m_timestamp, "timestamp"),
                                  visitor(m_multiGeometry, "multiGeometry"), // V9MM introduced multiGeometry instead of a single one
                                  visitor(m_visible, "visible"),
                                  visitor(m_constant1, "constant1"),
                                  visitor(m_constant2, "constant2"),
                                  visitor(m_constant3, "constant3"),
                                  visitor(m_nearestToponyms, "nearestToponyms"),
                                  visitor(m_properties, "properties"),
                                  VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  TrackData ConvertToLatestVersion() const
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry = mergeGeometry(m_multiGeometry);
    data.m_visible = m_visible;
    data.m_nearestToponyms = m_nearestToponyms;
    data.m_properties = m_properties;
    return data;
  }

  std::vector<MultiGeometry> m_multiGeometry;
};


// Contains the same sections as FileDataV8MM but with changed m_tracksData format
struct FileDataV9MM
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(FileDataV9MM, visitor(m_serverId, "serverId"),
                                  visitor(m_categoryData, "category"),
                                  visitor(m_bookmarksData, "bookmarks"),
                                  visitor(m_tracksData, "tracks"))

  bool operator==(FileDataV9MM const & data) const
  {
    return m_serverId == data.m_serverId && m_categoryData == data.m_categoryData &&
           m_bookmarksData == data.m_bookmarksData && m_tracksData == data.m_tracksData;
  }

  bool operator!=(FileDataV9MM const & data) const { return !operator==(data); }

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
    for (auto & t : m_tracksData)
      data.m_tracksData.emplace_back(t.ConvertToLatestVersion());

    return data;
  }

  // Device id (it will not be serialized in text files).
  std::string m_deviceId;
  // Server id.
  std::string m_serverId;
  // Category's data.
  CategoryDataV8MM m_categoryData;
  // Bookmarks collection.
  std::vector<BookmarkDataV8MM> m_bookmarksData;
  // Tracks collection.
  std::vector<TrackDataV9MM> m_tracksData;
};
}  // namespace kml
