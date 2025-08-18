#include "track_analyzing/log_parser.hpp"

#include "track_analyzing/exceptions.hpp"

#include "generator/borders.hpp"

#include "platform/platform.hpp"

#include "coding/hex.hpp"
#include "coding/traffic.hpp"

#include "geometry/mercator.hpp"

#include "base/file_name_utils.hpp"
#include "base/timer.hpp"

#include <cstdint>
#include <fstream>
#include <regex>
#include <unordered_set>

using namespace std;
using namespace track_analyzing;

namespace
{
vector<DataPoint> ReadDataPoints(string const & data)
{
  string const decoded = FromHex(data);
  vector<DataPoint> points;
  MemReaderWithExceptions memReader(decoded.data(), decoded.size());
  ReaderSource<MemReaderWithExceptions> src(memReader);

  try
  {
    coding::TrafficGPSEncoder::DeserializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, src, points);
  }
  catch (Reader::SizeException const & e)
  {
    points.clear();
    LOG(LERROR, ("DataPoint is corrupted. data:", data));
    LOG(LINFO, ("Continue reading..."));
  }
  return points;
}

class PointToMwmId final
{
public:
  PointToMwmId(shared_ptr<m4::Tree<routing::NumMwmId>> mwmTree, routing::NumMwmIds const & numMwmIds,
               string const & dataDir)
    : m_mwmTree(mwmTree)
  {
    numMwmIds.ForEachId([&](routing::NumMwmId numMwmId)
    {
      string const & mwmName = numMwmIds.GetFile(numMwmId).GetName();
      string const polyFile = base::JoinPath(dataDir, BORDERS_DIR, mwmName + BORDERS_EXTENSION);
      borders::LoadBorders(polyFile, m_borders[numMwmId]);
    });
  }

  routing::NumMwmId FindMwmId(m2::PointD const & point, routing::NumMwmId expectedId) const
  {
    if (expectedId != routing::kFakeNumMwmId && m2::RegionsContain(GetBorders(expectedId), point))
      return expectedId;

    routing::NumMwmId result = routing::kFakeNumMwmId;
    m2::RectD const rect = mercator::RectByCenterXYAndSizeInMeters(point, 1);
    m_mwmTree->ForEachInRect(rect, [&](routing::NumMwmId numMwmId)
    {
      if (result == routing::kFakeNumMwmId && m2::RegionsContain(GetBorders(numMwmId), point))
        result = numMwmId;
    });

    return result;
  }

private:
  vector<m2::RegionD> const & GetBorders(routing::NumMwmId numMwmId) const
  {
    auto it = m_borders.find(numMwmId);
    CHECK(it != m_borders.cend(), ());
    return it->second;
  }

  shared_ptr<m4::Tree<routing::NumMwmId>> m_mwmTree;
  unordered_map<routing::NumMwmId, vector<m2::RegionD>> m_borders;
};
}  // namespace

namespace track_analyzing
{
LogParser::LogParser(shared_ptr<routing::NumMwmIds> numMwmIds, unique_ptr<m4::Tree<routing::NumMwmId>> mwmTree,
                     string const & dataDir)
  : m_numMwmIds(std::move(numMwmIds))
  , m_mwmTree(std::move(mwmTree))
  , m_dataDir(dataDir)
{
  CHECK(m_numMwmIds, ());
  CHECK(m_mwmTree, ());
}

void LogParser::Parse(string const & logFile, MwmToTracks & mwmToTracks) const
{
  UserToTrack userToTrack;
  ParseUserTracks(logFile, userToTrack);
  SplitIntoMwms(userToTrack, mwmToTracks);
}

void LogParser::ParseUserTracks(string const & logFile, UserToTrack & userToTrack) const
{
  base::Timer timer;

  std::ifstream stream(logFile);
  if (!stream)
    MYTHROW(MessageException, ("Can't open file", logFile, "to parse tracks"));

  std::regex const base_regex(R"(.*(DataV0|CurrentData)\s+aloha_id\s*:\s*(\S+)\s+.*\|(\w+)\|)");
  std::unordered_set<string> usersWithOldVersion;
  uint64_t linesCount = 0;
  size_t pointsCount = 0;

  for (string line; getline(stream, line); ++linesCount)
  {
    std::smatch base_match;
    if (!std::regex_match(line, base_match, base_regex))
      continue;

    CHECK_EQUAL(base_match.size(), 4, ());

    string const version = base_match[1].str();
    string const userId = base_match[2].str();
    string const data = base_match[3].str();
    if (version != "CurrentData")
    {
      CHECK_EQUAL(version, "DataV0", ());
      usersWithOldVersion.insert(userId);
      continue;
    }

    auto const packet = ReadDataPoints(data);
    if (!packet.empty())
    {
      Track & track = userToTrack[userId];
      track.insert(track.end(), packet.cbegin(), packet.cend());
    }

    pointsCount += packet.size();
  };

  LOG(LINFO, ("Tracks parsing finished, elapsed:", timer.ElapsedSeconds(), "seconds, lines:", linesCount, ", points",
              pointsCount));
  LOG(LINFO, ("Users with current version:", userToTrack.size(), ", old version:", usersWithOldVersion.size()));
}

void LogParser::SplitIntoMwms(UserToTrack const & userToTrack, MwmToTracks & mwmToTracks) const
{
  base::Timer timer;

  PointToMwmId const pointToMwmId(m_mwmTree, *m_numMwmIds, m_dataDir);

  for (auto const & kv : userToTrack)
  {
    string const & user = kv.first;
    Track const & track = kv.second;

    routing::NumMwmId mwmId = routing::kFakeNumMwmId;
    for (DataPoint const & point : track)
    {
      mwmId = pointToMwmId.FindMwmId(mercator::FromLatLon(point.m_latLon), mwmId);
      if (mwmId != routing::kFakeNumMwmId)
        mwmToTracks[mwmId][user].push_back(point);
      else
        LOG(LERROR, ("Can't match mwm region for", point.m_latLon, ", user:", user));
    }
  }

  LOG(LINFO, ("Data was split into", mwmToTracks.size(), "mwms, elapsed:", timer.ElapsedSeconds(), "seconds"));
}
}  // namespace track_analyzing
