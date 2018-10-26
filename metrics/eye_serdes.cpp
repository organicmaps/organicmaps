#include "metrics/eye_serdes.hpp"
#include "metrics/eye_info.hpp"

#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include <sstream>
#include <string>
#include <type_traits>

namespace
{
struct MapObjectEvent
{
  DECLARE_VISITOR(visitor(m_bestPoiType, "best_type"), visitor(m_poiPos, "pos"),
                  visitor(m_event, "event"));

  std::string m_bestPoiType;
  ms::LatLon m_poiPos;
  eye::MapObject::Event m_event;
};
}  // namespace

namespace eye
{
// static
void Serdes::SerializeInfo(Info const & info, std::vector<int8_t> & result)
{
  result.clear();
  using Sink = MemWriter<std::vector<int8_t>>;
  Sink writer(result);

  WriteToSink(writer, static_cast<int8_t>(Info::GetVersion()));

  coding::SerializerJson<Sink> ser(writer);
  ser(info);
}

// static
void Serdes::DeserializeInfo(std::vector<int8_t> const & bytes, Info & result)
{
  MemReader reader(bytes.data(), bytes.size());
  NonOwningReaderSource source(reader);

  auto version = static_cast<int8_t>(Version::Unknown);
  ReadPrimitiveFromSource(source, version);
  if (version == static_cast<int8_t>(Version::V0))
  {
    try
    {
      // TODO: Use temporary object InfoV0 and implement method to convert it to Info,
      // TODO: when InfoV1 will be implemented.
      coding::DeserializerJson des(source);
      des(result);
    }
    catch (base::Json::Exception & ex)
    {
      LOG(LERROR, ("Cannot deserialize eye file. Exception:", ex.Msg(), "Version:", version,
                   "File content:", std::string(bytes.begin(), bytes.end())));
    }
    return;
  }

  MYTHROW(UnknownVersion, ("Unknown data version:", static_cast<int>(version)));
}

// static
void Serdes::SerializeMapObjects(MapObjects const & mapObjects, std::vector<int8_t> & result)
{
  result.clear();
  using Sink = MemWriter<std::vector<int8_t>>;

  Sink writer(result);
  std::string const nextLine = "\n";
  MapObjectEvent event;

  for (auto const & poi : mapObjects)
  {
    for (auto const & poiEvent : poi.second)
    {
      // Additional scope is added because of the coding::SerializerJson dumps result at destruction.
      {
        coding::SerializerJson<Sink> ser(writer);
        event.m_bestPoiType = poi.first.m_bestType;
        event.m_poiPos = poi.first.m_pos;
        event.m_event = poiEvent;
        ser(event);
      }
      writer.Write(nextLine.data(), nextLine.size());
    }
  }
}

// static
void Serdes::DeserializeMapObjects(std::vector<int8_t> const & bytes, MapObjects & result)
{
  MemReader reader(bytes.data(), bytes.size());
  NonOwningReaderSource source(reader);

  std::string tmp(bytes.begin(), bytes.end());
  std::istringstream is(tmp);

  std::string eventString;
  MapObjectEvent event;
  MapObject poi;

  try
  {
    while (getline(is, eventString))
    {
      if (eventString.empty())
        return;

      coding::DeserializerJson des(eventString);
      des(event);
      poi.m_bestType = event.m_bestPoiType;
      poi.m_pos = event.m_poiPos;

      auto it = result.find(poi);
      if (it == result.end())
      {
        std::vector<MapObject::Event> events = {event.m_event};
        result.emplace(poi, std::move(events));
      }
      else
      {
        it->second.emplace_back(event.m_event);
      }
    }
  }
  catch (base::Json::Exception & ex)
  {
    LOG(LERROR, ("Cannot deserialize map objects. Exception:", ex.Msg(), ". Event string:",
                 eventString, ". Content:", std::string(bytes.begin(), bytes.end())));
  }
}

// static
void Serdes::SerializeMapObjectEvent(MapObject const & poi, MapObject::Event const & poiEvent,
                                     std::vector<int8_t> & result)
{
  result.clear();

  using Sink = MemWriter<std::vector<int8_t>>;

  Sink writer(result);
  coding::SerializerJson<Sink> ser(writer);
  std::string const nextLine = "\n";

  MapObjectEvent event;
  event.m_bestPoiType = poi.m_bestType;
  event.m_poiPos = poi.m_pos;
  event.m_event = poiEvent;
  ser(event);
  writer.Write(nextLine.data(), nextLine.size());
}
}  // namespace eye
