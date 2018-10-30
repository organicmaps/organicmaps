#include "map/notifications/notification_queue_serdes.hpp"

#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

namespace notifications
{
// static
void QueueSerdes::Serialize(Queue const & queue, std::vector<int8_t> & result)
{
  result.clear();
  using Sink = MemWriter<std::vector<int8_t>>;
  Sink writer(result);

  WriteToSink(writer, static_cast<int8_t>(Queue::GetVersion()));

  coding::SerializerJson<Sink> ser(writer);
  ser(queue);
}

// static
void QueueSerdes::Deserialize(std::vector<int8_t> const & bytes, Queue & result)
{
  MemReader reader(bytes.data(), bytes.size());
  NonOwningReaderSource source(reader);

  auto version = static_cast<int8_t>(Version::Unknown);
  ReadPrimitiveFromSource(source, version);
  if (version == static_cast<int8_t>(Version::V0))
  {
    try
    {
      coding::DeserializerJson des(source);
      des(result);
    }
    catch (base::Json::Exception & ex)
    {
      LOG(LERROR, ("Cannot deserialize notification queue file. Exception:", ex.Msg(),
                   "Version:", version, "File content:", std::string(bytes.begin(), bytes.end())));
    }
    return;
  }

  MYTHROW(UnknownVersion, ("Unknown data version:", static_cast<int>(version)));
}
}  // namespace notifications
