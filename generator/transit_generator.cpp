#include "generator/transit_generator.hpp"

#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"
#include "generator/utils.hpp"

#include "routing/index_router.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/transit_types.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "indexer/mwm_set.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_writer.hpp"

#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include <vector>

#include "defines.hpp"

#include "3party/jansson/src/jansson.h"

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace std;
using namespace storage;

namespace
{
void LoadBorders(string const & dir, TCountryId const & countryId, vector<m2::RegionD> & borders)
{
  string const polyFile = my::JoinPath(dir, BORDERS_DIR, countryId + BORDERS_EXTENSION);
  borders.clear();
  osm::LoadBorders(polyFile, borders);
}

void FillOsmIdToFeatureIdsMap(string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & map)
{
  CHECK(ForEachOsmId2FeatureId(osmIdToFeatureIdsPath,
                               [&map](osm::Id const & osmId, uint32_t featureId) {
                                 map[osmId].push_back(featureId);
                               }),
        (osmIdToFeatureIdsPath));
}

string GetMwmPath(string const & mwmDir, TCountryId const & countryId)
{
  return my::JoinPath(mwmDir, countryId + DATA_FILE_EXTENSION);
}

/// \brief Calculates best pedestrian segment for every gate in |graphData.m_gates|.
/// The result of the calculation is set to |Gate::m_bestPedestrianSegment| of every gate
/// from |graphData.m_gates|.
/// \note All gates in |graphData.m_gates| must have a valid |m_point| field before the call.
void CalculateBestPedestrianSegments(string const & mwmPath, TCountryId const & countryId,
                                     GraphData & graphData)
{
  // Creating IndexRouter.
  SingleMwmIndex index(mwmPath);

  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoReader(GetPlatform());
  CHECK(infoGetter, ());

  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) {
    return infoGetter->GetRegionCountryId(pt);
  };

  auto const getMwmRectByName = [&](string const & c) -> m2::RectD {
    CHECK_EQUAL(countryId, c, ());
    return infoGetter->GetLimitRectForLeaf(c);
  };

  CHECK_EQUAL(index.GetMwmId().GetInfo()->GetType(), MwmInfo::COUNTRY, ());
  auto numMwmIds = make_shared<NumMwmIds>();
  numMwmIds->RegisterFile(CountryFile(countryId));

  // Note. |indexRouter| is valid while |index| is valid.
  IndexRouter indexRouter(VehicleType::Pedestrian, false /* load altitudes */,
                          CountryParentNameGetterFn(), countryFileGetter, getMwmRectByName,
                          numMwmIds, MakeNumMwmTree(*numMwmIds, *infoGetter),
                          traffic::TrafficCache(), index.GetIndex());
  auto worldGraph = indexRouter.MakeSingleMwmWorldGraph();

  // Looking for the best segment for every gate.
  auto const & gates = graphData.GetGates();
  for (size_t i = 0; i < gates.size(); ++i)
  {
    auto const & gate = gates[i];
    if (countryFileGetter(gate.GetPoint()) != countryId)
      continue;
    // Note. For pedestrian routing all the segments are considered as twoway segments so
    // IndexRouter.FindBestSegment() method finds the same segment for |isOutgoing| == true
    // and |isOutgoing| == false.
    Segment bestSegment;
    try
    {
      if (countryFileGetter(gate.GetPoint()) != countryId)
        continue;
      if (indexRouter.FindBestSegment(gate.GetPoint(), m2::PointD::Zero() /* direction */,
                                      true /* isOutgoing */, *worldGraph, bestSegment))
      {
        CHECK_EQUAL(bestSegment.GetMwmId(), 0, ());
        graphData.SetGateBestPedestrianSegment(i, SingleMwmSegment(
            bestSegment.GetFeatureId(), bestSegment.GetSegmentIdx(), bestSegment.IsForward()));
      }
    }
    catch (MwmIsNotAliveException const & e)
    {
      LOG(LCRITICAL, ("Point of a gate belongs to the processed mwm:", countryId, ","
          "but the mwm is not alive. Gate:", gate, e.what()));
    }
    catch (RootException const & e)
    {
      LOG(LCRITICAL, ("Exception while looking for the best segment of a gate. CountryId:",
          countryId, ". Gate:", gate, e.what()));
    }
  }
}
}  // namespace

namespace routing
{
namespace transit
{
void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping,
                         string const & transitJsonPath, GraphData & data)
{
  Platform::EFileType fileType;
  Platform::EError const errCode = Platform::GetFileType(transitJsonPath, fileType);
  CHECK_EQUAL(errCode, Platform::EError::ERR_OK, ("Transit graph was not found:", transitJsonPath));
  CHECK_EQUAL(fileType, Platform::EFileType::FILE_TYPE_REGULAR,
              ("Transit graph was not found:", transitJsonPath));

  string jsonBuffer;
  try
  {
    GetPlatform().GetReader(transitJsonPath)->ReadAsString(jsonBuffer);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Can't open", transitJsonPath, e.what()));
  }

  my::Json root(jsonBuffer.c_str());
  CHECK(root.get() != nullptr, ("Cannot parse the json file:", transitJsonPath));

  data.Clear();
  try
  {
    data.DeserializeFromJson(root, mapping);
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Exception while parsing transit graph json. Json file path:", transitJsonPath, e.what()));
  }
}

void ProcessGraph(string const & mwmPath, TCountryId const & countryId,
                  OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, GraphData & data)
{
  CalculateBestPedestrianSegments(mwmPath, countryId, data);
  data.Sort();
  CHECK(data.IsValid(), (mwmPath));
}

void BuildTransit(string const & mwmDir, TCountryId const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir)
{
  LOG(LINFO, ("Building transit section for", countryId));
  Platform::FilesList graphFiles;
  Platform::GetFilesByExt(my::AddSlashIfNeeded(transitDir), TRANSIT_FILE_EXTENSION, graphFiles);

  string const mwmPath = GetMwmPath(mwmDir, countryId);
  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);
  vector<m2::RegionD> mwmBorders;
  LoadBorders(mwmDir, countryId, mwmBorders);

  GraphData jointData;
  for (auto const & fileName : graphFiles)
  {
    auto const filePath = my::JoinPath(transitDir, fileName);
    GraphData data;
    DeserializeFromJson(mapping, filePath, data);
    // @todo(bykoianko) Json should be clipped on feature generation step. It's much more efficient.
    data.ClipGraph(mwmBorders);
    jointData.AppendTo(data);
  }

  if (jointData.IsEmpty())
    return; // Empty transit section.

  ProcessGraph(mwmPath, countryId, mapping, jointData);
  CHECK(jointData.IsValid(), (mwmPath, countryId));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(TRANSIT_FILE_TAG);
  jointData.Serialize(writer);
}
}  // namespace transit
}  // namespace routing
