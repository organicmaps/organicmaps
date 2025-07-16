#pragma once

#include "kml/types.hpp"
#include "kml/types_v8mm.hpp"

namespace kml
{

MultiGeometry mergeGeometry(std::vector<MultiGeometry> && aGeometries);

struct TrackDataV9MM : TrackDataV8MM
{
  DECLARE_VISITOR_AND_DEBUG_PRINT(
      TrackDataV9MM, visitor(m_id, "id"), visitor(m_localId, "localId"), visitor(m_name, "name"),
      visitor(m_description, "description"), visitor(m_layers, "layers"), visitor(m_timestamp, "timestamp"),
      visitor(m_multiGeometry, "multiGeometry"),  // V9MM introduced multiGeometry instead of a single one
      visitor(m_visible, "visible"), visitor(m_constant1, "constant1"), visitor(m_constant2, "constant2"),
      visitor(m_constant3, "constant3"), visitor(m_nearestToponyms, "nearestToponyms"),
      visitor(m_properties, "properties"), VISITOR_COLLECTABLE)

  DECLARE_COLLECTABLE(LocalizableStringIndex, m_name, m_description, m_nearestToponyms, m_properties)

  TrackData ConvertToLatestVersion()
  {
    TrackData data;
    data.m_id = m_id;
    data.m_localId = m_localId;
    data.m_name = m_name;
    data.m_description = m_description;
    data.m_layers = m_layers;
    data.m_timestamp = m_timestamp;
    data.m_geometry = mergeGeometry(std::move(m_multiGeometry));
    data.m_visible = m_visible;
    data.m_nearestToponyms = m_nearestToponyms;
    data.m_properties = m_properties;
    return data;
  }

  std::vector<MultiGeometry> m_multiGeometry;
};

// Contains the same sections as FileDataV8MM but with changed m_tracksData format
using FileDataV9MM = FileDataMMImpl<TrackDataV9MM>;

}  // namespace kml
