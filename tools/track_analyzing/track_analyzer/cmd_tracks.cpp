#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "traffic/speed_groups.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/vehicle_model.hpp"

#include "routing/edge_estimator.hpp"
#include "routing/geometry.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace track_analyzing
{
using namespace routing;
using namespace std;

class TrackStats final
{
public:
  void AddUsers(uint64_t numUsers) { m_numUsers += numUsers; }
  void AddTracks(uint64_t numTracks) { m_numTracks += numTracks; }
  void AddPoints(uint64_t numPoints) { m_numPoints += numPoints; }

  void Add(TrackStats const & rhs)
  {
    m_numUsers += rhs.m_numUsers;
    m_numTracks += rhs.m_numTracks;
    m_numPoints += rhs.m_numPoints;
  }

  string GetSummary() const
  {
    ostringstream out;
    out << "users: " << m_numUsers << ", tracks: " << m_numTracks << ", points: " << m_numPoints;
    return out.str();
  }

  bool IsEmpty() const { return m_numPoints == 0; }

private:
  uint64_t m_numUsers = 0;
  uint64_t m_numTracks = 0;
  uint64_t m_numPoints = 0;
};

class ErrorStat final
{
public:
  void Add(double value)
  {
    m_count += 1.0;
    m_squares += value * value;
    m_min = min(m_min, value);
    m_max = max(m_max, value);
  }

  double GetStdDev() const
  {
    CHECK_GREATER(m_count, 1.0, ());
    return std::sqrt(m_squares / (m_count - 1.0));
  }

  string GetStdDevString() const
  {
    if (m_count <= 1.0)
      return "N/A";

    return to_string(GetStdDev());
  }

  double GetMin() const { return m_min; }
  double GetMax() const { return m_max; }

private:
  double m_count = 0.0;
  double m_squares = 0.0;
  double m_min = numeric_limits<double>::max();
  double m_max = numeric_limits<double>::min();
};

bool TrackHasTrafficPoints(MatchedTrack const & track)
{
  for (MatchedTrackPoint const & point : track)
  {
    size_t const index = static_cast<size_t>(point.GetDataPoint().m_traffic);
    CHECK_LESS(index, static_cast<size_t>(traffic::SpeedGroup::Count), ());
    if (traffic::kSpeedGroupThresholdPercentage[index] != 100)
      return true;
  }

  return false;
}

double EstimateDuration(MatchedTrack const & track, shared_ptr<EdgeEstimator> estimator, Geometry & geometry)
{
  double result = 0.0;
  Segment segment;

  for (MatchedTrackPoint const & point : track)
  {
    if (point.GetSegment() == segment)
      continue;

    segment = point.GetSegment();
    result +=
        estimator->CalcSegmentWeight(segment, geometry.GetRoad(segment.GetFeatureId()), EdgeEstimator::Purpose::ETA);
  }

  return result;
}

void CmdTracks(string const & filepath, string const & trackExtension, StringFilter mwmFilter, StringFilter userFilter,
               TrackFilter const & filter, bool noTrackLogs, bool noMwmLogs, bool noWorldLogs)
{
  storage::Storage storage;
  auto numMwmIds = CreateNumMwmIds(storage);

  map<string, TrackStats> mwmToStats;
  ErrorStat absoluteError;
  ErrorStat relativeError;

  auto processMwm = [&](string const & mwmName, UserToMatchedTracks const & userToMatchedTracks)
  {
    if (mwmFilter(mwmName))
      return;

    TrackStats & mwmStats = mwmToStats[mwmName];

    shared_ptr<VehicleModelInterface> vehicleModel = CarModelFactory({}).GetVehicleModelForCountry(mwmName);

    Geometry geometry(GeometryLoader::CreateFromFile(GetCurrentVersionMwmFile(storage, mwmName), vehicleModel));

    shared_ptr<EdgeEstimator> estimator = EdgeEstimator::Create(
        VehicleType::Car, *vehicleModel, nullptr /* trafficStash */, nullptr /* dataSource */, nullptr /* numMwmIds */);

    for (auto const & it : userToMatchedTracks)
    {
      string const & user = it.first;
      if (userFilter(user))
        continue;

      vector<MatchedTrack> const & tracks = it.second;
      if (!noTrackLogs)
        cout << mwmName << ", user: " << user << endl;

      bool thereAreUnfilteredTracks = false;
      for (MatchedTrack const & track : tracks)
      {
        DataPoint const & start = track.front().GetDataPoint();
        DataPoint const & finish = track.back().GetDataPoint();

        double const length = CalcTrackLength(track, geometry);
        uint64_t const duration = finish.m_timestamp - start.m_timestamp;
        double const speed = CalcSpeedKMpH(length, duration);
        bool const hasTrafficPoints = TrackHasTrafficPoints(track);

        if (!filter.Passes(duration, length, speed, hasTrafficPoints))
          continue;

        double const estimatedDuration = EstimateDuration(track, estimator, geometry);
        double const timeError = estimatedDuration - static_cast<double>(duration);

        if (!noTrackLogs)
        {
          cout << fixed << setprecision(1) << "  points: " << track.size() << ", length: " << length
               << ", duration: " << duration << ", estimated duration: " << estimatedDuration << ", speed: " << speed
               << ", traffic: " << hasTrafficPoints
               << ", departure: " << base::SecondsSinceEpochToString(start.m_timestamp)
               << ", arrival: " << base::SecondsSinceEpochToString(finish.m_timestamp)
               << setprecision(numeric_limits<double>::max_digits10) << ", start: " << start.m_latLon.m_lat << ", "
               << start.m_latLon.m_lon << ", finish: " << finish.m_latLon.m_lat << ", " << finish.m_latLon.m_lon
               << endl;
        }

        mwmStats.AddTracks(1);
        mwmStats.AddPoints(track.size());
        absoluteError.Add(timeError);
        relativeError.Add(timeError / static_cast<double>(duration));
        thereAreUnfilteredTracks = true;
      }

      if (thereAreUnfilteredTracks)
        mwmStats.AddUsers(1);
    }
  };

  auto processFile = [&](string const & filename, MwmToMatchedTracks const & mwmToMatchedTracks)
  {
    LOG(LINFO, ("Processing", filename));
    ForTracksSortedByMwmName(mwmToMatchedTracks, *numMwmIds, processMwm);
  };

  ForEachTrackFile(filepath, trackExtension, numMwmIds, processFile);

  if (!noMwmLogs)
  {
    cout << endl;
    for (auto const & it : mwmToStats)
      if (!it.second.IsEmpty())
        cout << it.first << ": " << it.second.GetSummary() << endl;
  }

  if (!noWorldLogs)
  {
    TrackStats worldStats;
    for (auto const & it : mwmToStats)
      worldStats.Add(it.second);

    cout << endl << "World: " << worldStats.GetSummary() << endl;
    cout << fixed << setprecision(1) << "Absolute error: deviation: " << absoluteError.GetStdDevString()
         << ", min: " << absoluteError.GetMin() << ", max: " << absoluteError.GetMax() << endl;
    cout << fixed << setprecision(3) << "Relative error: deviation: " << relativeError.GetStdDevString()
         << ", min: " << relativeError.GetMin() << ", max: " << relativeError.GetMax() << endl;
  }
}
}  // namespace track_analyzing
