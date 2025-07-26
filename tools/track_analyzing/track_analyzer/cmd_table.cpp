#include "track_analyzing/track_analyzer/crossroad_checker.hpp"

#include "track_analyzing/track_analyzer/utils.hpp"

#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "routing/city_roads.hpp"
#include "routing/data_source.hpp"
#include "routing/geometry.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/maxspeeds.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "traffic/speed_groups.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/features_vector.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_reader.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/sunrise_sunset.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "defines.hpp"

namespace track_analyzing
{
using namespace routing;
using namespace std;

namespace
{
MaxspeedType constexpr kMaxspeedTopBound = 200;
auto constexpr kValidTrafficValue = traffic::SpeedGroup::G5;

string TypeToString(uint32_t type)
{
  if (type == 0)
    return "unknown-type";

  return classif().GetReadableObjectName(type);
}

bool DayTimeToBool(DayTimeType type)
{
  switch (type)
  {
  case DayTimeType::Day:
  case DayTimeType::PolarDay: return true;
  case DayTimeType::Night:
  case DayTimeType::PolarNight: return false;
  }

  UNREACHABLE();
}

class CarModelTypes final
{
public:
  CarModelTypes()
  {
    auto const & cl = classif();

    for (auto const & speed : CarModel::GetOptions())
      m_hwtags.push_back(cl.GetTypeForIndex(static_cast<uint32_t>(speed.m_type)));

    for (auto const & surface : CarModel::GetSurfaces())
      m_surfaceTags.push_back(cl.GetTypeByPath(surface.m_type));
  }

  struct Type
  {
    bool operator<(Type const & rhs) const
    {
      return tie(m_hwType, m_surfaceType) < tie(rhs.m_hwType, rhs.m_surfaceType);
    }

    bool operator==(Type const & rhs) const { return tie(m_hwType, m_surfaceType) == tie(rhs.m_hwType, m_surfaceType); }

    bool operator!=(Type const & rhs) const { return !(*this == rhs); }

    string GetSummary() const
    {
      ostringstream out;
      out << TypeToString(m_hwType) << "," << TypeToString(m_surfaceType);
      return out.str();
    }

    uint32_t m_hwType = 0;
    uint32_t m_surfaceType = 0;
  };

  Type GetType(feature::TypesHolder const & types) const
  {
    Type ret;
    for (uint32_t type : m_hwtags)
    {
      if (types.Has(type))
      {
        ret.m_hwType = type;
        break;
      }
    }

    for (uint32_t type : m_surfaceTags)
    {
      if (types.Has(type))
      {
        ret.m_surfaceType = type;
        break;
      }
    }

    return ret;
  }

private:
  vector<uint32_t> m_hwtags;
  vector<uint32_t> m_surfaceTags;
};

struct RoadInfo
{
  bool operator==(RoadInfo const & rhs) const
  {
    return tie(m_type, m_maxspeedKMpH, m_isCityRoad, m_isOneWay) ==
           tie(rhs.m_type, rhs.m_maxspeedKMpH, rhs.m_isCityRoad, rhs.m_isOneWay);
  }

  bool operator!=(RoadInfo const & rhs) const { return !(*this == rhs); }

  bool operator<(RoadInfo const & rhs) const
  {
    return tie(m_type, m_maxspeedKMpH, m_isCityRoad, m_isOneWay) <
           tie(rhs.m_type, rhs.m_maxspeedKMpH, rhs.m_isCityRoad, rhs.m_isOneWay);
  }

  string GetSummary() const
  {
    ostringstream out;
    out << TypeToString(m_type.m_hwType) << "," << TypeToString(m_type.m_surfaceType) << "," << m_maxspeedKMpH << ","
        << m_isCityRoad << "," << m_isOneWay;

    return out.str();
  }

  CarModelTypes::Type m_type;
  MaxspeedType m_maxspeedKMpH = kInvalidSpeed;
  bool m_isCityRoad = false;
  bool m_isOneWay = false;
};

class MoveType final
{
public:
  MoveType() = default;

  MoveType(RoadInfo const & roadType, traffic::SpeedGroup speedGroup, DataPoint const & dataPoint)
    : m_roadInfo(roadType)
    , m_speedGroup(speedGroup)
    , m_latLon(dataPoint.m_latLon)
  {
    m_isDayTime = DayTimeToBool(GetDayTime(dataPoint.m_timestamp, m_latLon.m_lat, m_latLon.m_lon));
  }

  bool operator==(MoveType const & rhs) const
  {
    return tie(m_roadInfo, m_speedGroup) == tie(rhs.m_roadInfo, rhs.m_speedGroup);
  }

  bool operator<(MoveType const & rhs) const
  {
    auto const lhsGroup = base::Underlying(m_speedGroup);
    auto const rhsGroup = base::Underlying(rhs.m_speedGroup);
    return tie(m_roadInfo, lhsGroup) < tie(rhs.m_roadInfo, rhsGroup);
  }

  bool IsValid() const
  {
    // In order to collect cleaner data we don't use speed group lower than G5.
    return m_roadInfo.m_type.m_hwType != 0 && m_roadInfo.m_type.m_surfaceType != 0 &&
           m_speedGroup == kValidTrafficValue;
  }

  string GetSummary() const
  {
    ostringstream out;
    out << m_roadInfo.GetSummary() << "," << m_isDayTime << "," << m_latLon.m_lat << " " << m_latLon.m_lon;

    return out.str();
  }

private:
  RoadInfo m_roadInfo;
  traffic::SpeedGroup m_speedGroup = traffic::SpeedGroup::Unknown;
  ms::LatLon m_latLon;
  bool m_isDayTime = false;
};

class SpeedInfo final
{
public:
  void Add(double distance, uint64_t time, IsCrossroadChecker::CrossroadInfo const & crossroads,
           uint32_t dataPointsNumber)
  {
    m_totalDistance += distance;
    m_totalTime += time;
    IsCrossroadChecker::MergeCrossroads(crossroads, m_crossroads);
    m_dataPointsNumber += dataPointsNumber;
  }

  string GetSummary() const
  {
    ostringstream out;
    out << m_totalDistance << "," << m_totalTime << "," << CalcSpeedKMpH(m_totalDistance, m_totalTime) << ",";

    for (size_t i = 0; i < m_crossroads.size(); ++i)
    {
      out << m_crossroads[i];
      if (i != m_crossroads.size() - 1)
        out << ",";
    }

    return out.str();
  }

  uint32_t GetDataPointsNumber() const { return m_dataPointsNumber; }

private:
  double m_totalDistance = 0.0;
  uint64_t m_totalTime = 0;
  IsCrossroadChecker::CrossroadInfo m_crossroads{};
  uint32_t m_dataPointsNumber = 0;
};

class MoveTypeAggregator final
{
public:
  void Add(MoveType && moveType, IsCrossroadChecker::CrossroadInfo const & crossroads,
           MatchedTrack::const_iterator begin, MatchedTrack::const_iterator end, Geometry & geometry)
  {
    if (begin + 1 >= end)
      return;

    auto const & beginDataPoint = begin->GetDataPoint();
    auto const & endDataPoint = (end - 1)->GetDataPoint();
    uint64_t const time = endDataPoint.m_timestamp - beginDataPoint.m_timestamp;

    if (time == 0)
    {
      LOG(LWARNING, ("Track with the same time at the beginning and at the end. Beginning:", beginDataPoint.m_latLon,
                     " End:", endDataPoint.m_latLon, " Timestamp:", beginDataPoint.m_timestamp,
                     " Segment:", begin->GetSegment()));
      return;
    }

    double const length = CalcSubtrackLength(begin, end, geometry);
    m_moveInfos[moveType].Add(length, time, crossroads, static_cast<uint32_t>(distance(begin, end)));
  }

  string GetSummary(string const & user, string const & mwmName, string const & countryName, Stats & stats) const
  {
    ostringstream out;
    for (auto const & it : m_moveInfos)
    {
      if (!it.first.IsValid())
        continue;

      out << user << "," << mwmName << "," << it.first.GetSummary() << "," << it.second.GetSummary() << '\n';

      stats.AddDataPoints(mwmName, countryName, it.second.GetDataPointsNumber());
    }

    return out.str();
  }

private:
  map<MoveType, SpeedInfo> m_moveInfos;
};

class MatchedTrackPointToMoveType final
{
public:
  MatchedTrackPointToMoveType(FilesContainerR const & container, VehicleModelInterface & vehicleModel)
    : m_featuresVector(container)
    , m_vehicleModel(vehicleModel)
  {
    if (container.IsExist(CITY_ROADS_FILE_TAG))
      m_cityRoads.Load(container.GetReader(CITY_ROADS_FILE_TAG));

    if (container.IsExist(MAXSPEEDS_FILE_TAG))
      m_maxspeeds.Load(container.GetReader(MAXSPEEDS_FILE_TAG));
  }

  MoveType GetMoveType(MatchedTrackPoint const & point)
  {
    auto const & dataPoint = point.GetDataPoint();
    return MoveType(GetRoadInfo(point.GetSegment()), static_cast<traffic::SpeedGroup>(dataPoint.m_traffic), dataPoint);
  }

private:
  RoadInfo GetRoadInfo(Segment const & segment)
  {
    auto const featureId = segment.GetFeatureId();
    if (featureId == m_prevFeatureId)
      return m_prevRoadInfo;

    auto feature = m_featuresVector.GetVector().GetByIndex(featureId);
    CHECK(feature, ());

    auto const maxspeed = m_maxspeeds.GetMaxspeed(featureId);
    auto const maxspeedValueKMpH =
        maxspeed.IsValid() ? min(maxspeed.GetSpeedKmPH(segment.IsForward()), kMaxspeedTopBound) : kInvalidSpeed;

    m_prevFeatureId = featureId;

    feature::TypesHolder const types(*feature);
    m_prevRoadInfo = {m_carModelTypes.GetType(types), maxspeedValueKMpH, m_cityRoads.IsCityRoad(featureId),
                      m_vehicleModel.IsOneWay(types)};
    return m_prevRoadInfo;
  }

  FeaturesVectorTest m_featuresVector;
  VehicleModelInterface & m_vehicleModel;
  CarModelTypes const m_carModelTypes;
  CityRoads m_cityRoads;
  Maxspeeds m_maxspeeds;
  uint32_t m_prevFeatureId = numeric_limits<uint32_t>::max();
  RoadInfo m_prevRoadInfo;
};
}  // namespace

void CmdTagsTable(string const & filepath, string const & trackExtension, StringFilter mwmFilter,
                  StringFilter userFilter)
{
  WriteCsvTableHeader(cout);

  storage::Storage storage;
  storage.RegisterAllLocalMaps();
  FrozenDataSource dataSource;
  auto numMwmIds = CreateNumMwmIds(storage);

  Stats stats;
  auto processMwm = [&](string const & mwmName, UserToMatchedTracks const & userToMatchedTracks)
  {
    if (mwmFilter(mwmName))
      return;

    auto const countryName = storage.GetTopmostParentFor(mwmName);
    auto const carModelFactory = make_shared<CarModelFactory>(VehicleModelFactory::CountryParentNameGetterFn{});
    shared_ptr<VehicleModelInterface> vehicleModel = carModelFactory->GetVehicleModelForCountry(mwmName);
    string const mwmFile = GetCurrentVersionMwmFile(storage, mwmName);
    MatchedTrackPointToMoveType pointToMoveType(FilesContainerR(make_unique<FileReader>(mwmFile)), *vehicleModel);
    Geometry geometry(GeometryLoader::CreateFromFile(mwmFile, vehicleModel));
    auto const vehicleType = VehicleType::Car;
    auto const edgeEstimator =
        EdgeEstimator::Create(vehicleType, *vehicleModel, nullptr /* trafficStash */, &dataSource, numMwmIds);

    MwmDataSource routingSource(dataSource, numMwmIds);
    auto indexGraphLoader =
        IndexGraphLoader::Create(vehicleType, false /* loadAltitudes */, carModelFactory, edgeEstimator, routingSource);

    platform::CountryFile const countryFile(mwmName);
    auto localCountryFile = storage.GetLatestLocalFile(countryFile);
    CHECK(localCountryFile, ("Can't find latest country file for", countryFile.GetName()));
    if (!dataSource.IsLoaded(countryFile))
    {
      auto registerResult = dataSource.Register(*localCountryFile);
      CHECK_EQUAL(registerResult.second, MwmSet::RegResult::Success, ("Can't register mwm", countryFile.GetName()));
    }

    auto const mwmId = numMwmIds->GetId(countryFile);
    IsCrossroadChecker checker(indexGraphLoader->GetIndexGraph(mwmId), geometry);

    for (auto const & kv : userToMatchedTracks)
    {
      string const & user = kv.first;
      if (userFilter(user))
        continue;

      for (auto const & track : kv.second)
      {
        if (track.size() <= 1)
          continue;

        MoveTypeAggregator aggregator;
        IsCrossroadChecker::CrossroadInfo info = {};
        for (auto subtrackBegin = track.begin(); subtrackBegin != track.end();)
        {
          auto moveType = pointToMoveType.GetMoveType(*subtrackBegin);
          auto prev = subtrackBegin;
          auto end = subtrackBegin + 1;
          // Splitting track with points where MoveType is changed.
          while (end != track.end() && pointToMoveType.GetMoveType(*end) == moveType)
          {
            IsCrossroadChecker::MergeCrossroads(checker(prev->GetSegment(), end->GetSegment()), info);
            prev = end;
            ++end;
          }

          // If it's not the end of the track than it could be a crossroad.
          if (end != track.end())
            IsCrossroadChecker::MergeCrossroads(checker(prev->GetSegment(), end->GetSegment()), info);

          aggregator.Add(std::move(moveType), info, subtrackBegin, end, geometry);
          subtrackBegin = end;
          info.fill(0);
        }

        auto const summary = aggregator.GetSummary(user, mwmName, countryName, stats);
        if (!summary.empty())
          cout << summary;
      }
    }
  };

  auto processTrack = [&](string const & filename, MwmToMatchedTracks const & mwmToMatchedTracks)
  {
    LOG(LINFO, ("Processing", filename));
    ForTracksSortedByMwmName(mwmToMatchedTracks, *numMwmIds, processMwm);
  };

  ForEachTrackFile(filepath, trackExtension, numMwmIds, processTrack);

  stats.Log();
}
}  // namespace track_analyzing
