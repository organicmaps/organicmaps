#include <generator/borders_generator.hpp>
#include <coding/file_name_utils.hpp>
#include <generator/borders_loader.hpp>
#include <regex>
#include <fstream>
#include <unordered_set>
#include <platform/platform.hpp>
#include <base/timer.hpp>
#include <coding/hex.hpp>
#include "track_analyzing/log_parser.hpp"
#include <cstdint>
#include <geometry/mercator.hpp>

using namespace std;
using namespace tracking;

namespace
{
vector<DataPoint> ReadDataPoints(string const & data)
{
  string const decoded = FromHex(data);
  vector<uint8_t> buffer;
  for (auto c : decoded)
    buffer.push_back(static_cast<uint8_t>(c));

  vector<DataPoint> points;
  MemReader memReader(buffer.data(), buffer.size());
  ReaderSource<MemReader> src(memReader);
  coding::TrafficGPSEncoder::DeserializeDataPoints(1 /* version */, src, points);
  return points;
}

class PointToMwmId final
{
public:
  PointToMwmId(shared_ptr<m4::Tree<routing::NumMwmId>> mwmTree,
               routing::NumMwmIds const & numMwmIds, string const & dataDir)
    : m_mwmTree(mwmTree)
  {
    numMwmIds.ForEachId([&](routing::NumMwmId numMwmId) {
      string const & mwmName = numMwmIds.GetFile(numMwmId).GetName();
      string const polyFile = my::JoinPath(dataDir, BORDERS_DIR, mwmName + BORDERS_EXTENSION);
      osm::LoadBorders(polyFile, m_borders[numMwmId]);
    });
  }

  routing::NumMwmId FindMwmId(m2::PointD const & point, routing::NumMwmId expectedId) const
  {
    if (expectedId != routing::kFakeNumMwmId && m2::RegionsContain(GetBorders(expectedId), point))
      return expectedId;

    routing::NumMwmId result = routing::kFakeNumMwmId;
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(point, 1);
    m_mwmTree->ForEachInRect(rect, [&](routing::NumMwmId numMwmId) {
      if (m2::RegionsContain(GetBorders(numMwmId), point))
      {
        result = numMwmId;
        return;
      }
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

namespace tracking
{
LogParser::LogParser(shared_ptr<routing::NumMwmIds> numMwmIds,
                     unique_ptr<m4::Tree<routing::NumMwmId>> mwmTree, string const & dataDir)
  : m_numMwmIds(move(numMwmIds)), m_mwmTree(move(mwmTree)), m_dataDir(dataDir)
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
  my::Timer timer;

  std::ifstream stream(logFile);
  CHECK(stream.is_open(), ("Can't open file", logFile));

  std::regex const base_regex(
      ".*(DataV0|CurrentData)\\s+aloha_id\\s*:\\s*(\\S+)\\s+.*\\|(\\w+)\\|");
  std::unordered_set<string> usersWithOldVersion;
  size_t linesCount = 0;
  size_t pointsCount = 0;

  for (string line; getline(stream, line);)
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
      track.insert(track.end(), packet.begin(), packet.end());
    }

    ++linesCount;
    pointsCount += packet.size();
  };

  LOG(LINFO, ("Tracks parsing finished, elapsed:", timer.ElapsedSeconds(), "seconds, lines:",
              linesCount, ", points", pointsCount));
  LOG(LINFO, ("Users with current version:", userToTrack.size(), ", old version:",
              usersWithOldVersion.size()));
}

void LogParser::SplitIntoMwms(UserToTrack const & userToTrack, MwmToTracks & mwmToTracks) const
{
  my::Timer timer;

  PointToMwmId const pointToMwmId(m_mwmTree, *m_numMwmIds, m_dataDir);

  for (auto & it : userToTrack)
  {
    string const & user = it.first;
    Track const & track = it.second;

    routing::NumMwmId mwmId = routing::kFakeNumMwmId;
    for (DataPoint const & point : track)
    {
      mwmId = pointToMwmId.FindMwmId(MercatorBounds::FromLatLon(point.m_latLon), mwmId);
      if (mwmId != routing::kFakeNumMwmId)
        mwmToTracks[mwmId][user].push_back(point);
      else
        LOG(LERROR, ("Can't match mwm region for", point.m_latLon, ", user:", user));
    }
  }

  LOG(LINFO, ("Data was splitted into", mwmToTracks.size(), "mwms, elapsed:",
              timer.ElapsedSeconds(), "seconds"));
}
}  // namespace tracking
