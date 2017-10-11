#include "generator/transit_generator.hpp"

#include "generator/osm_id.hpp"
#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"

#include "routing_common/transit_serdes.hpp"
#include "routing_common/transit_types.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"

#include <functional>
#include <memory>
#include <utility>

#include "3party/jansson/src/jansson.h"

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace std;

namespace
{
/// Extracts the file name from |filePath| and drops all extensions.
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

template <class Item>
void DeserializeFromJson(my::Json const & root, string const & key, vector<Item> & items)
{
  items.clear();
  DeserializerFromJson deserializer(root.get());
  deserializer(items, key.c_str());
}

void DeserializeGatesFromJson(my::Json const & root, string const & mwmDir, string const & countryId,
                              vector<Gate> & gates)
{
  DeserializeFromJson(root, "gates", gates);

  // Creating IndexRouter.
  SingleMwmIndex singleIndex(my::JoinFoldersToPath(mwmDir, countryId + DATA_FILE_EXTENSION));

  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(infoGetter, ());

  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
    return infoGetter->GetRegionCountryId(pt);
  };

  auto const getMwmRectByName = [&](string const & c) -> m2::RectD {
    CHECK_EQUAL(countryId, c, ());
    return infoGetter->GetLimitRectForLeaf(c);
  };

  CHECK_EQUAL(singleIndex.GetMwmId().GetInfo()->GetType(), MwmInfo::COUNTRY, ());
  auto numMwmIds = make_shared<NumMwmIds>();
  numMwmIds->RegisterFile(CountryFile(countryId));

  // Note. |indexRouter| is valid until |index| is valid.
  IndexRouter indexRouter(VehicleType::Pedestrian, false /* load altitudes */,
                          CountryParentNameGetterFn(), countryFileGetter, getMwmRectByName,
                          numMwmIds, MakeNumMwmTree(*numMwmIds, *infoGetter),
                          traffic::TrafficCache(), singleIndex.GetIndex());

  // Looking for the best segment for every gate.
  for (auto & gate : gates)
  {
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      if (indexRouter.FindBestSegmentAtSingleMwm(gate.GetPoint(),
                                                 m2::PointD::Zero() /* direction */,
                                                 true /* isOutgoing */, bestSegment))
      {
        CHECK_EQUAL(bestSegment.GetMwmId(), 0,
                    ("Best segment num mwm id ==", bestSegment.GetMwmId(), ", but should be zero"));
        gate.SetBestPedestrianSegment(SingleMwmSegment(
            bestSegment.GetFeatureId(), bestSegment.GetSegmentIdx(), bestSegment.IsForward()));
      }
    }
    catch (RootException const & e)
    {
      LOG(LDEBUG, ("Point of a gate belongs to several mwm or doesn't belong any mwm. gate:", gate,
                   e.what(), e.Msg()));
    }
  }
}

/// \brief Reads from |root| (json) and serializes an array to |serializer|.
template <class Item>
void SerializeObject(my::Json const & root, string const & key, Serializer<FileWriter> & serializer)
{
  vector<Item> items;
  DeserializeFromJson(root, key, items);

  serializer(items);
}
}  // namespace

namespace routing
{
namespace transit
{
// DeserializerFromJson ---------------------------------------------------------------------------
void DeserializerFromJson::operator()(m2::PointD & p, char const * name)
{
  json_t * pointItem = nullptr;
  if (name == nullptr)
    pointItem = m_node; // Array item case
  else
    pointItem = my::GetJSONObligatoryField(m_node, name);

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
  string const graphFullPath = my::JoinFoldersToPath(transitDir, countryId + TRANSIT_FILE_EXTENSION);

  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(graphFullPath, fileType);
  if (errCode != Platform::EError::ERR_OK || fileType != Platform::EFileType::FILE_TYPE_REGULAR)
  {
    LOG(LINFO, ("For mwm:", mwmPath, TRANSIT_FILE_EXTENSION, "file not found"));
    return;
  }

  // @TODO(bykoianko) In the future transit edges which cross mwm border will be split in the generator. Then
  // routing will support cross mwm transit routing. In current version every json with transit graph
  // should have a special name: <country id>.transit.json.

  LOG(LINFO, (TRANSIT_FILE_TAG, "section is being created. Country id:", countryId, ". Based on:", graphFullPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(graphFullPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & ex)
  {
    LOG(LCRITICAL, ("Can't open", graphFullPath, ex.what()));
  }

  // @TODO(bykoianko) If it's necessary to parse an integer jansson parser keeps it in long long value.
  // It's not good because osm id and stop id are uint64_t. This should be solved before continue writing
  // transit jansson parsing. According to C++ signed long long is not smaller than long and is at least 64 bits.
  // So as a variant before saving to json osm id and stop id should be converted to signed long long and
  // then after reading at generator they should be converted back.
  // @TODO(bykoianko) |osmId| should be converted to feature id while deserialing from json.
  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", graphFullPath));

  // Note. |gates| has to be deserialized from json before to start writing transit section to mwm since
  // the mwm is used to filled |gates|.
  vector<Gate> gates;
  DeserializeGatesFromJson(root, my::GetDirectory(mwmPath), countryId, gates);

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(TRANSIT_FILE_TAG);

  TransitHeader header;

  auto const startOffset = w.Pos();
  Serializer<FileWriter> serializer(w);
  header.Visit(serializer);

  SerializeObject<Stop>(root, "stops", serializer);
  header.m_gatesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  serializer(gates);
  header.m_edgesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Edge>(root, "edges", serializer);
  header.m_transfersOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Transfer>(root, "transfers", serializer);
  header.m_linesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Line>(root, "lines", serializer);
  header.m_shapesOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Shape>(root, "shapes", serializer);
  header.m_networksOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  SerializeObject<Network>(root, "networks", serializer);
  header.m_endOffset = base::checked_cast<uint32_t>(w.Pos() - startOffset);

  // Rewriting header info.
  auto const endOffset = w.Pos();
  w.Seek(startOffset);
  header.Visit(serializer);
  w.Seek(endOffset);
  LOG(LINFO, (TRANSIT_FILE_TAG, "section is ready. Header:", header));
}
}  // namespace transit
}  // namespace routing
