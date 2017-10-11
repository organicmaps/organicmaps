#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "routing/geometry.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/vehicle_model.hpp"

#include "traffic/speed_groups.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "coding/file_name_utils.hpp"

#include <iostream>

using namespace routing;
using namespace std;
using namespace track_analyzing;

namespace
{
string RoadTypeToString(uint32_t type)
{
  if (type == 0)
    return "unknown-type";

  return classif().GetReadableObjectName(type);
}

class CarModelTypes final
{
public:
  CarModelTypes()
  {
    for (auto const & additionalTag : CarModel::GetAdditionalTags())
      m_tags.push_back(classif().GetTypeByPath(additionalTag.m_hwtag));

    for (auto const & speedForType : CarModel::GetLimits())
    {
      vector<string> path;
      for (char const * type : speedForType.m_types)
        path.push_back(string(type));

      m_tags.push_back(classif().GetTypeByPath(path));
    }
  }

  uint32_t GetType(FeatureType const & feature) const
  {
    feature::TypesHolder holder(feature);
    for (uint32_t type : m_tags)
    {
      if (holder.Has(type))
        return type;
    }

    return 0;
  }

private:
  vector<uint32_t> m_tags;
};

class MoveType final
{
public:
  MoveType() = default;

  MoveType(uint32_t roadType, traffic::SpeedGroup speedGroup)
    : m_roadType(roadType), m_speedGroup(speedGroup)
  {
  }

  bool operator==(MoveType const & rhs) const
  {
    return m_roadType == rhs.m_roadType && m_speedGroup == rhs.m_speedGroup;
  }

  bool operator<(MoveType const & rhs) const
  {
    if (m_roadType != rhs.m_roadType)
      return m_roadType < rhs.m_roadType;

    return m_speedGroup < rhs.m_speedGroup;
  }

  string ToString() const
  {
    ostringstream out;
    out << RoadTypeToString(m_roadType) << "*" << traffic::DebugPrint(m_speedGroup);
    return out.str();
  }

private:
  uint32_t m_roadType = 0;
  traffic::SpeedGroup m_speedGroup = traffic::SpeedGroup::Unknown;
};

class MoveInfo final
{
public:
  void Add(double distance, uint64_t time)
  {
    m_totalDistance += distance;
    m_totalTime += time;
  }

  void Add(MoveInfo const & rhs) { Add(rhs.m_totalDistance, rhs.m_totalTime); }

  double GetDistance() const { return m_totalDistance; }
  uint64_t GetTime() const { return m_totalTime; }

private:
  double m_totalDistance = 0.0;
  uint64_t m_totalTime = 0;
};

class MoveTypeAggregator final
{
public:
  void Add(MoveType const moveType, MatchedTrack::const_iterator begin,
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

  string GetSummary() const
  {
    ostringstream out;
    out << std::fixed << std::setprecision(1);

    bool firstIteration = true;
    for (auto it : m_moveInfos)
    {
      MoveInfo const & speedInfo = it.second;
      if (firstIteration)
        firstIteration = false;
      else
        out << " ";

      out << it.first.ToString() << ": " << speedInfo.GetDistance() << " / " << speedInfo.GetTime();
    }

    return out.str();
  }

private:
  map<MoveType, MoveInfo> m_moveInfos;
};

class MatchedTrackPointToMoveType final
{
public:
  MatchedTrackPointToMoveType(string const & mwmFile)
    : m_featuresVector(FilesContainerR(make_unique<FileReader>(mwmFile)))
  {
  }

  MoveType GetMoveType(MatchedTrackPoint const & point)
  {
    return MoveType(GetRoadType(point.GetSegment().GetFeatureId()),
                    static_cast<traffic::SpeedGroup>(point.GetDataPoint().m_traffic));
  }

private:
  uint32_t GetRoadType(uint32_t featureId)
  {
    if (featureId == m_prevFeatureId)
      return m_prevRoadType;

    FeatureType feature;
    m_featuresVector.GetVector().GetByIndex(featureId, feature);

    m_prevFeatureId = featureId;
    m_prevRoadType = m_carModelTypes.GetType(feature);
    return m_prevRoadType;
  }

  CarModelTypes const m_carModelTypes;
  FeaturesVectorTest m_featuresVector;
  uint32_t m_prevFeatureId = numeric_limits<uint32_t>::max();
  uint32_t m_prevRoadType = numeric_limits<uint32_t>::max();
};
}  // namespace

namespace track_analyzing
{
void CmdTagsTable(string const & filepath, string const & trackExtension, StringFilter mwmFilter,
                  StringFilter userFilter)
{
  cout << "mwm user track_idx start length time speed ... type: meters / seconds" << endl;

  storage::Storage storage;
  auto numMwmIds = CreateNumMwmIds(storage);

  auto processMwm = [&](string const & mwmName, UserToMatchedTracks const & userToMatchedTracks) {
    if (mwmFilter(mwmName))
      return;

    shared_ptr<VehicleModelInterface> vehicleModel =
        CarModelFactory({}).GetVehicleModelForCountry(mwmName);
    string const mwmFile = GetCurrentVersionMwmFile(storage, mwmName);
    MatchedTrackPointToMoveType pointToMoveType(mwmFile);
    Geometry geometry(GeometryLoader::CreateFromFile(mwmFile, vehicleModel));

    for (auto const & kv : userToMatchedTracks)
    {
      string const & user = kv.first;
      if (userFilter(user))
        continue;

      for (size_t trackIdx = 0; trackIdx < kv.second.size(); ++trackIdx)
      {
        MatchedTrack const & track = kv.second[trackIdx];
        uint64_t const start = track.front().GetDataPoint().m_timestamp;
        uint64_t const timeElapsed = track.back().GetDataPoint().m_timestamp - start;
        double const length = CalcTrackLength(track, geometry);
        double const speed = CalcSpeedKMpH(length, timeElapsed);

        MoveTypeAggregator aggregator;

        for (auto subTrackBegin = track.begin(); subTrackBegin != track.end();)
        {
          auto moveType = pointToMoveType.GetMoveType(*subTrackBegin);
          auto subTrackEnd = subTrackBegin + 1;
          while (subTrackEnd != track.end() &&
                 pointToMoveType.GetMoveType(*subTrackEnd) == moveType)
            ++subTrackEnd;

          aggregator.Add(moveType, subTrackBegin, subTrackEnd, geometry);
          subTrackBegin = subTrackEnd;
        }

        cout << mwmName << " " << user << " " << trackIdx << " "
             << my::SecondsSinceEpochToString(start) << " " << length << " " << timeElapsed << " "
             << speed << " " << aggregator.GetSummary() << endl;
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
