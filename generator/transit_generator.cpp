#include "generator/transit_generator.hpp"

#include "generator/osm_id.hpp"

#include "routing_common/transit_header.hpp"
#include "routing_common/transit_stop.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

#include "3party/jansson/myjansson.hpp"

using namespace platform;
using namespace std;

namespace
{
using namespace routing;
using namespace routing::transit;

/// \brief Fills Stop instance based on a json field. |node| should point at jansson item at
/// array:
///  "stops": [
///  {
///    "id": 343259523,
///    "line_ids": [
///        19207936,
///        19207937
///    ],
///    "osm_id": 4611686018770647427,
///    "point": {
///      "x": 27.4970954,
///      "y": 64.20146835878187
///    },
///    "title_anchors": []
///  },
/// ...
/// ]
void ReadJsonArrayItem(json_struct_t * node, Stop & stop)
{
  json_t * idItem = my::GetJSONObligatoryField(node, "id");
  StopId id;
  FromJSON(idItem, id);

  json_t * osmIdItem = my::GetJSONObligatoryField(node, "osm_id");
  json_int_t osmId;
  FromJSON(osmIdItem, osmId);
  // @TODO(bykoianko) |osmId| should be converted to feature id here.
  FeatureId const featureId = 0;

  vector<LineId> lineIds;
  FromJSONObject(node, "line_ids", lineIds);

  json_t * pointItem = my::GetJSONObligatoryField(node, "point");
  CHECK(json_is_object(pointItem), ());
  json_t * xItem = my::GetJSONObligatoryField(pointItem, "x");
  m2::PointD point;
  FromJSON(xItem, point.x);
  json_t * yItem = my::GetJSONObligatoryField(pointItem, "y");
  FromJSON(yItem, point.y);

  stop = Stop(id, featureId, lineIds, point);
}

// @TODO(bykoianko) ReadJsonArrayItem(...) methods for the other transit graph structures should be added here.

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

template <class Item, class Sink>
void SerializeObject(my::Json const & root, string const & key, Sink & sink)
{
  json_t const * const stops = json_object_get(root.get(), key.c_str());
  CHECK(stops, ());
  size_t const sz = json_array_size(stops);
  for (size_t i = 0; i < sz; ++i)
  {
    Item item;
    ReadJsonArrayItem(json_array_get(stops, i), item);
    item.Serialize(sink);
  }
}
}  // namespace

namespace routing
{
namespace transit
{
string GetCountryId(string & graphCountryId);
void BuildTransit(string const & mwmPath, string const & transitDir)
{
  // This method is under construction and should not be used for building production mwm sections.
  NOTIMPLEMENTED();

  string const countryId = GetFileName(mwmPath);
  LOG(LINFO, ("countryId:", countryId));

  Platform::FilesList filesList;
  Platform::GetFilesByExt(transitDir, ".json", filesList);

  LOG(LINFO,
      ("mwm path:", mwmPath, ", directory with transit:", transitDir, "filesList:", filesList));

  for (string const & graphFileName : filesList)
  {
    string const graphFullPath = my::JoinFoldersToPath(transitDir, graphFileName);

    Platform::EFileType fileType;
    Platform::EError errCode = Platform::GetFileType(graphFullPath, fileType);
    CHECK_EQUAL(errCode, Platform::EError::ERR_OK,
                ("File is not found:", graphFullPath, ", errCode:", errCode));
    CHECK_EQUAL(fileType, Platform::EFileType::FILE_TYPE_REGULAR,
                ("File is not found:", graphFullPath, ", fileType:", fileType));

    // @TODO(bykoianko) In the future transit edges which cross mwm border will be split in the generator. Then
    // routing will support cross mwm transit routing. In current version every json with transit graph
    // should have a special name: <country id>.transit.json.
    string const graphCountryId = GetFileName(graphFileName);
    LOG(LINFO, ("graphCountryId:", graphCountryId));

    if (graphCountryId != countryId)
      continue;

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
    my::Json root(jsonBuffer.c_str());
    CHECK(root.get() != nullptr, ("Cannot parse the json file:", graphFullPath));

    FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
    FileWriter w = cont.GetWriter(TRANSIT_FILE_TAG);

    TransitHeader header;

    auto const startOffset = w.Pos();
    header.Serialize(w);

    SerializeObject<Stop>(root, "stops", w);
    header.m_gatesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

    // @TODO(bykoianko) It's necessary to serialize other transit graph data here.

    w.WritePaddingByEnd(8);
    header.m_endOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

    // Rewriting header info.
    auto const endOffset = w.Pos();
    w.Seek(startOffset);
    header.Serialize(w);
    w.Seek(endOffset);
    LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. The size is", header.m_endOffset));
  }
}
}  // namespace transit
}  // namespace routing
