#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

using namespace routing;
using namespace std;

namespace track_analyzing
{
void CmdCppTrack(string const & trackFile, string const & mwmName, string const & user, size_t trackIdx)
{
  storage::Storage storage;
  auto const numMwmIds = CreateNumMwmIds(storage);
  MwmToMatchedTracks mwmToMatchedTracks;
  ReadTracks(numMwmIds, trackFile, mwmToMatchedTracks);

  MatchedTrack const & track = GetMatchedTrack(mwmToMatchedTracks, *numMwmIds, mwmName, user, trackIdx);

  auto const backupPrecision = cout.precision();
  cout.precision(8);
  for (MatchedTrackPoint const & point : track)
    cout << "  {" << point.GetDataPoint().m_latLon.m_lat << ", " << point.GetDataPoint().m_latLon.m_lon << "}," << endl;
  cout.precision(backupPrecision);
}
}  // namespace track_analyzing
