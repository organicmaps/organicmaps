#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "3party/gflags/src/gflags/gflags_declare.h"
#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(csv_path, "",
              "Path to csv file with user in following format: mwm id (string), aloha id (string), "
              "latitude of the first coord (double), longitude of the first coord (double), "
              "timestamp in seconds (int), latitude of the second coord (double) and so on.");
DEFINE_bool(deviations, false, "Print deviations in meters for all extrapolations.");
DEFINE_int32(extrapolation_frequency, 10, "The number extrapolations in a second.");

using namespace std;

namespace
{
struct GpsPoint
{
  GpsPoint(double lat, double lon, double timestamp)
    : m_lat(lat), m_lon(lon), m_timestamp(timestamp)
  {
  }

  double m_lat;
  double m_lon;
  double m_timestamp;
};

using Track = vector<GpsPoint>;
using Tracks = vector<Track>;

bool GetString(stringstream & lineStream, string & result)
{
  if (!lineStream.good())
    return false;
  getline(lineStream, result, ',');
  return true;
}

bool GetDouble(stringstream & lineStream, double & result)
{
  string strResult;
  if (!GetString(lineStream, strResult))
    return false;
  return strings::to_double(strResult, result);
}

bool GetUint64(stringstream & lineStream, uint64_t & result)
{
  string strResult;
  if (!GetString(lineStream, strResult))
    return false;
  return strings::to_uint64(strResult, result);
}

bool GetGpsPoint(stringstream & lineStream, double & lat, double & lon, uint64_t & timestamp)
{
  if (!GetDouble(lineStream, lat))
    return false;

  if (!GetDouble(lineStream, lon))
    return false;

  return GetUint64(lineStream, timestamp);
}

/// \brief Fills |tracks| based on file |pathToCsv| content. File |pathToCsv| should be
/// a text file with following lines:
/// <Mwm name (country id)>, <Aloha id>,
/// <Latitude of the first point>, <Longitude of the first point>, <Timestamp in seconds of the first point>,
/// <Latitude of the second point> and so on.
bool Parse(string const & pathToCsv, Tracks & tracks)
{
  tracks.clear();

  std::ifstream csvStream(pathToCsv);
  if (!csvStream.is_open())
    return false;

  while (!csvStream.eof())
  {
    string line;
    getline(csvStream, line);
    stringstream lineStream(line);
    string dummy;
    GetString(lineStream, dummy); // mwm id
    GetString(lineStream, dummy); // aloha id

    Track track;
    while (!lineStream.eof())
    {
      double lat;
      double lon;
      uint64_t timestamp;
      if (!GetGpsPoint(lineStream, lat, lon, timestamp))
        return false;
      track.emplace_back(lat, lon, static_cast<double>(timestamp));
    }
    tracks.push_back(move(track));
  }
  csvStream.close();
  return true;
}
} // namespace

/// \brief This benchmark is written to estimate how LinearExtrapolation() extrapolates real users tracks.
/// The idea behind the test is to measure the distance between extrapolated location and real track.
int main(int argc, char * argv[])
{
  google::SetUsageMessage(
      "Location extrapolation benchmark. Calculates mathematical expectation and dispersion for "
      "all extrapolation deviations form track passed in csv_path.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_csv_path.empty())
  {
    LOG(LERROR, ("Path to csv file with user tracks should be set."));
    return -1;
  }

  Tracks tracks;
  if (!Parse(FLAGS_csv_path, tracks))
  {
    LOG(LERROR, ("An error while parsing", FLAGS_csv_path, "file. Please check if it has a correct format."));
    return -1;
  }

  size_t trackPointNum = 0;
  for (auto const & t : tracks)
    trackPointNum += t.size();

  double trackLengths = 0;
  for (auto const & t : tracks)
  {
    double trackLenM = 0;
    for (size_t j = 1; j < t.size(); ++j)
      trackLenM += ms::DistanceOnEarth(ms::LatLon(t[j - 1].m_lat, t[j - 1].m_lon), ms::LatLon(t[j].m_lat, t[j].m_lon));
    trackLengths += trackLenM;
  }

  LOG(LINFO, ("General tracks stats. Number of tracks:", tracks.size(), "Track points:",
              trackPointNum, "Points per track in average:", trackPointNum / tracks.size(),
              "Average track length:", trackLengths / tracks.size(), "meters"));

  // @TODO(bykoianko) Parsed user track is placed to |tracks|. Then LinearExtrapolation()
  // shall be used.

  return 0;
}
