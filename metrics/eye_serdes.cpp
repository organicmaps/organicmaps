#include "metrics/eye_serdes.hpp"

#include "coding/reader.hpp"
#include "coding/serdes_json.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/logging.hpp"

#include <string>
#include <type_traits>

namespace eye
{
// static
void Serdes::Serialize(Info const & info, std::vector<int8_t> & result)
{
  result.clear();
  using Sink = MemWriter<std::vector<int8_t>>;
  Sink writer(result);

  WriteToSink(writer, static_cast<int8_t>(Info::GetVersion()));

  coding::SerializerJson<Sink> ser(writer);
  ser(info);
}

// static
void Serdes::Deserialize(std::vector<int8_t> const & bytes, Info & result)
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
    catch (my::Json::Exception & ex)
    {
      LOG(LERROR, ("Cannot deserialize eye file. Exception:", ex.Msg(), "Version:", version,
                   "File content:", bytes));
    }
    return;
  }

  MYTHROW(UnknownVersion, ("Unknown data version:", static_cast<int>(version)));
}
}  // namespace eye
