#include "generator/transit_generator.hpp"

#include "generator/osm_id.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/platform.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include "3party/jansson/myjansson.hpp"

using namespace platform;
using namespace std;

namespace
{
using namespace routing;
using namespace routing::transit;

/// \returns file name without extension by a file path if a file name which have zero, one of several extensions.
/// For example,
/// GetFileName("Russia_Nizhny Novgorod Oblast.transit.json") returns "Russia_Nizhny Novgorod Oblast"
/// GetFileName("Russia_Nizhny Novgorod Oblast.mwm") returns "Russia_Nizhny Novgorod Oblast"
string GetFileName(string const & filePath)
{
  string name = filePath;
  my::GetNameFromFullPath(name);

  string nameWithExt;
  do
  {
    nameWithExt = name;
    my::GetNameWithoutExt(name);
  }
  while (nameWithExt != name);

  return name;
}

/// \brief Reads from |root| (json) and serialize an array to |serializer|.
template <class Item>
void SerializeObject(my::Json const & root, string const & key, Serializer<FileWriter> & serializer)
{
  vector<Item> items;

  DeserializerFromJson deserializer(root.get());
  deserializer(items, key.c_str());

  for (auto const & s : items)
    s.Visit(serializer);
}
}  // namespace

namespace routing
{
namespace transit
{
// DeserializerFromJson ---------------------------------------------------------------------------
void DeserializerFromJson::operator()(m2::PointD & p, char const * name)
{
  json_t * pointItem = my::GetJSONOptionalField(m_node, name);
  if (pointItem == nullptr)
    return;
  CHECK(json_is_object(pointItem), ());
  FromJSONObject(pointItem, "x", p.x);
  FromJSONObject(pointItem, "y", p.y);
}

void BuildTransit(string const & mwmPath, string const & transitDir)
{
  LOG(LERROR, ("This method is under construction and should not be used for building production mwm "
      "sections."));
  NOTIMPLEMENTED();

  string const countryId = GetFileName(mwmPath);
  LOG(LINFO, ("countryId:", countryId));

  string const graphFullPath = my::JoinFoldersToPath(transitDir, countryId + ".transit.json");

  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(graphFullPath, fileType);
  if (errCode != Platform::EError::ERR_OK || fileType != Platform::EFileType::FILE_TYPE_REGULAR)
  {
    LOG(LINFO, ("For mwm:", mwmPath, ".transit.json file not found"));
    return;
  }

  // @TODO(bykoianko) In the future transit edges which cross mwm border will be split in the generator. Then
  // routing will support cross mwm transit routing. In current version every json with transit graph
  // should have a special name: <country id>.transit.json.

  LOG(LINFO, ("Creating", TRANSIT_FILE_TAG, "section for:", countryId, "based on", graphFullPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(graphFullPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Can't open", graphFullPath, ex.what()));
  }

  // @TODO(bykoianko) If it's necessary to parse an integer jansson parser keeps it to time long long value.
  // It's not good because osm id and stop id are uint64_t. This should be solve before continue writing
  // transit jansson parsing. According to C++ signed long long is not smaller than long and at least 64 bits.
  // So as a variant before saving to json osm id and stop id should be converted to signed long long and
  // then after reading at generator they should be converted back.
  // @TODO(bykoianko) |osmId| should be converted to feature id while deserialing from json.
  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", graphFullPath));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(TRANSIT_FILE_TAG);

  TransitHeader header;

  auto const startOffset = w.Pos();
  Serializer<FileWriter> serializer(w);
  header.Visit(serializer);

  SerializeObject<Stop>(root, "stops", serializer);
  header.m_gatesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  // @TODO(bykoianko) It's necessary to serialize other transit graph data here.

  w.WritePaddingByEnd(8);
  header.m_endOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  // Rewriting header info.
  auto const endOffset = w.Pos();
  w.Seek(startOffset);
  header.Visit(serializer);
  w.Seek(endOffset);
  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. The size is", header.m_endOffset));
}
}  // namespace transit
}  // namespace routing
