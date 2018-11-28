#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "routing/city_roads.hpp"
#include "routing/geometry.hpp"
#include "routing/maxspeeds.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/maxspeed_conversion.hpp"
#include "routing_common/vehicle_model.hpp"

#include "traffic/speed_groups.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/features_vector.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"

#include "geometry/latlon.hpp"

#include "base/assert.hpp"
#include "base/sunrise_sunset.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "defines.hpp"

using namespace routing;
using namespace std;
using namespace track_analyzing;

namespace
{
uint16_t constexpr kWalkMaxspeedKMpHValue = 10;
uint16_t constexpr kNoneMaxspeedKMpHValue = 200;

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
  case DayTimeType::PolarDay:
    return true;
  case DayTimeType::Night:
  case DayTimeType::PolarNight:
    return false;
  }

  UNREACHABLE();
}

class CarModelTypes final
{
public:
  CarModelTypes()
  {
    for (auto const & additionalTag : CarModel::GetAdditionalTags())
      m_hwtags.push_back(classif().GetTypeByPath(additionalTag.m_hwtag));

    for (auto const & speedForType : CarModel::GetLimits())
      m_hwtags.push_back(classif().GetTypeByPath(speedForType.m_types));

    for (auto const & surface : CarModel::GetSurfaces())
      m_surfaceTags.push_back(classif().GetTypeByPath(surface.m_types));
  }

  struct Type
  {
    bool operator<(Type const & rhs) const
    {
      if (m_hwType != rhs.m_hwType)
        return m_hwType < rhs.m_hwType;

      return m_surfaceType < rhs.m_surfaceType;
    }

    bool operator==(Type const & rhs) const
    {
      return m_hwType == rhs.m_hwType && m_surfaceType == rhs.m_surfaceType;
    }

    bool operator!=(Type const & rhs) const
    {
      return !(*this == rhs);
    }

    string GetSummary() const
    {
      ostringstream out;
      out << TypeToString(m_hwType) << "," << TypeToString(m_surfaceType);
      return out.str();
    }

    uint32_t m_hwType = 0;
    uint32_t m_surfaceType = 0;
  };

  Type GetType(FeatureType & feature) const
  {
    Type ret;
    feature::TypesHolder holder(feature);
    for (uint32_t type : m_hwtags)
    {
      if (holder.Has(type))
      {
        ret.m_hwType = type;
        break;
      }
    }

    for (uint32_t type : m_surfaceTags)
    {
      if (holder.Has(type))
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
    return m_type == rhs.m_type &&
           m_maxspeedKMpH == rhs.m_maxspeedKMpH &&
           m_isCityRoad == rhs.m_isCityRoad &&
           m_isOneWay == rhs.m_isOneWay;
  }

  bool operator!=(RoadInfo const & rhs) const
  {
    return !(*this == rhs);
  }

  bool operator<(RoadInfo const & rhs) const
  {
    if (m_type != rhs.m_type)
      return m_type < rhs.m_type;

    if (m_maxspeedKMpH != rhs.m_maxspeedKMpH)
      return m_maxspeedKMpH < rhs.m_maxspeedKMpH;

    if (m_isCityRoad != rhs.m_isCityRoad)
      return m_isCityRoad < rhs.m_isCityRoad;

    return m_isOneWay < rhs.m_isOneWay;
  }

  string GetSummary() const
  {
    ostringstream out;
    out << TypeToString(m_type.m_hwType) << ","
        << TypeToString(m_type.m_surfaceType) << ","
        << m_maxspeedKMpH << ","
        << m_isCityRoad << ","
        << m_isOneWay;

    return out.str();
  }

  CarModelTypes::Type m_type;
  uint16_t m_maxspeedKMpH = kInvalidSpeed;
  bool m_isCityRoad = false;
  bool m_isOneWay = false;
};

class MoveType final
{
public:
  MoveType() = default;

  MoveType(RoadInfo const & roadType, traffic::SpeedGroup speedGroup, DataPoint const & dataPoint)
    : m_roadInfo(roadType), m_speedGroup(speedGroup), m_latLon(dataPoint.m_latLon)
  {
    m_isDayTime = DayTimeToBool(GetDayTime(dataPoint.m_timestamp, m_latLon.lat, m_latLon.lon));
  }

  bool operator==(MoveType const & rhs) const
  {
    return m_roadInfo == rhs.m_roadInfo && m_speedGroup == rhs.m_speedGroup && m_isDayTime == rhs.m_isDayTime;
  }

  bool operator<(MoveType const & rhs) const
  {
    if (m_roadInfo != rhs.m_roadInfo)
      return m_roadInfo < rhs.m_roadInfo;

    if (m_speedGroup != rhs.m_speedGroup)
      return m_speedGroup < rhs.m_speedGroup;

    return m_isDayTime < rhs.m_isDayTime;
  }

  bool IsValid() const
  {
    // In order to collect cleaner data we don't use speed group lower than G5.
    return m_roadInfo.m_type.m_hwType != 0 &&
           m_roadInfo.m_type.m_surfaceType != 0 &&
           m_speedGroup == traffic::SpeedGroup::G5;
  }

  string GetSummary() const
  {
    ostringstream out;
    out << m_roadInfo.GetSummary() << ","
        << m_isDayTime << ","
        << m_latLon.lat << " " << m_latLon.lon;

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
  void Add(double distance, uint64_t time)
  {
    m_totalDistance += distance;
    m_totalTime += time;
  }

  void Add(SpeedInfo const & rhs) { Add(rhs.m_totalDistance, rhs.m_totalTime); }

  string GetSummary() const
  {
    ostringstream out;
    out << m_totalDistance << "," << m_totalTime << "," << CalcSpeedKMpH(m_totalDistance, m_totalTime);
    return out.str();
  }

private:
  double m_totalDistance = 0.0;
  uint64_t m_totalTime = 0;
};

class MoveTypeAggregator final
{
public:
  void Add(MoveType && moveType, MatchedTrack::const_iterator begin,
           MatchedTrack::const_iterator end, Geometry & geometry)
  {
    if (begin + 1 >= end)
      return;

    uint64_t const time = (end - 1)->GetDataPoint().m_timestamp - begin->GetDataPoint().m_timestamp;
    double const length = CalcSubtrackLength(begin, end, geometry);
    m_moveInfos[moveType].Add(length, time);
  }

  void Add(MoveTypeAggregator const & rhs)
  {
    for (auto it : rhs.m_moveInfos)
      m_moveInfos[it.first].Add(it.second);
  }

  string GetSummary(string const & user, string const & mwm) const
  {
    ostringstream out;
    for (auto const & it : m_moveInfos)
    {
      if (!it.first.IsValid())
        continue;

      out << user << "," << mwm << "," << it.first.GetSummary() << "," << it.second.GetSummary() << '\n';
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
    : m_featuresVector(container), m_vehicleModel(vehicleModel)
  {
    if (container.IsExist(CITY_ROADS_FILE_TAG))
      LoadCityRoads(container.GetFileName(), container.GetReader(CITY_ROADS_FILE_TAG), m_cityRoads);

    if (container.IsExist(MAXSPEEDS_FILE_TAG))
      LoadMaxspeeds(container.GetReader(MAXSPEEDS_FILE_TAG), m_maxspeeds);
  }

  MoveType GetMoveType(MatchedTrackPoint const & point)
  {
    auto const & dataPoint = point.GetDataPoint();
    return MoveType(GetRoadInfo(point.GetSegment()),
                    static_cast<traffic::SpeedGroup>(dataPoint.m_traffic),
                    dataPoint);
  }

private:
  RoadInfo GetRoadInfo(Segment const & segment)
  {
    auto const featureId = segment.GetFeatureId();
    if (featureId == m_prevFeatureId)
      return m_prevRoadInfo;

    FeatureType feature;
    m_featuresVector.GetVector().GetByIndex(featureId, feature);

    auto const maxspeed = m_maxspeeds.GetMaxspeed(featureId);
    auto maxspeedValueKMpH = kInvalidSpeed;
    if (maxspeed.IsValid())
    {
      maxspeedValueKMpH = maxspeed.GetSpeedKmPH(segment.IsForward());
      if (maxspeedValueKMpH == kNoneMaxSpeed)
        maxspeedValueKMpH = kNoneMaxspeedKMpHValue;

      if (maxspeedValueKMpH == kWalkMaxSpeed)
        maxspeedValueKMpH = kWalkMaxspeedKMpHValue;

      maxspeedValueKMpH = min(maxspeedValueKMpH, kNoneMaxspeedKMpHValue);
    }

    m_prevFeatureId = featureId;
    m_prevRoadInfo = {m_carModelTypes.GetType(feature),
                      maxspeedValueKMpH,
                      m_cityRoads.IsCityRoad(featureId),
                      m_vehicleModel.IsOneWay(feature)};

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

namespace track_analyzing
{
void CmdTagsTable(string const & filepath, string const & trackExtension, StringFilter mwmFilter,
                  StringFilter userFilter)
{
  cout << "user,mwm,hw type,surface type,maxspeed km/h,is city road,is one way,is day,lat lon,distance,time,mean speed km/h\n";

  storage::Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  auto numMwmIds = CreateNumMwmIds(storage);

  auto processMwm = [&](string const & mwmName, UserToMatchedTracks const & userToMatchedTracks) {
    if (mwmFilter(mwmName))
      return;

    shared_ptr<VehicleModelInterface> vehicleModel =
        CarModelFactory({}).GetVehicleModelForCountry(mwmName);
    string const mwmFile = GetCurrentVersionMwmFile(storage, mwmName);
    MatchedTrackPointToMoveType pointToMoveType(FilesContainerR(make_unique<FileReader>(mwmFile)), *vehicleModel);
    Geometry geometry(GeometryLoader::CreateFromFile(mwmFile, vehicleModel));

    for (auto const & kv : userToMatchedTracks)
    {
      string const & user = kv.first;
      if (userFilter(user))
        continue;

      for (size_t trackIdx = 0; trackIdx < kv.second.size(); ++trackIdx)
      {
        MatchedTrack const & track = kv.second[trackIdx];
        if (track.size() <= 1)
          continue;

        MoveTypeAggregator aggregator;
        for (auto subTrackBegin = track.begin(); subTrackBegin != track.end();)
        {
          auto moveType = pointToMoveType.GetMoveType(*subTrackBegin);
          auto subTrackEnd = subTrackBegin + 1;
          while (subTrackEnd != track.end() &&
                 pointToMoveType.GetMoveType(*subTrackEnd) == moveType)
            ++subTrackEnd;

          aggregator.Add(move(moveType), subTrackBegin, subTrackEnd, geometry);
          subTrackBegin = subTrackEnd;
        }

        auto const summary = aggregator.GetSummary(user, mwmName);
        if (!summary.empty())
          cout << summary;
      }
    }
  };

  auto processTrack = [&](string const & filename, MwmToMatchedTracks const & mwmToMatchedTracks) {
    LOG(LINFO, ("Processing", filename));
    ForTracksSortedByMwmName(mwmToMatchedTracks, *numMwmIds, processMwm);
  };

  ForEachTrackFile(filepath, trackExtension, numMwmIds, processTrack);
}
}  // namespace track_analyzing
