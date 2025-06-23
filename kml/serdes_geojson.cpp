#include "kml/serdes_geojson.hpp"

#include "coding/serdes_json.hpp"

#include "base/visitor.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <map>
#include <string>
#include <unordered_set>

namespace kml
{
namespace geojson
{

void GeojsonWriter::Write(FileData const & fileData)
{
  GeoJsonData data;
  coding::SerializerJson<Writer> ser(m_writer);
  ser(data);
}

/*template <typename ReaderType>
void GeojsonParser::Parse(ReaderType const & reader)
{
  geojson::GeoJsonData data;
  NonOwningReaderSource source(reader);
  coding::DeserializerJson des(source);
  des(data);

  // Copy bookmarks from parsed 'data' into m_fileData.
  //TODO

  // Copy tracks from parsed 'data' into m_fileData.
  //TODO
}*/

}  // namespace geojson
}  // namespace kml
