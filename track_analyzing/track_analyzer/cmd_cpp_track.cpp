#include "track_analyzing/track.hpp"
#include "track_analyzing/utils.hpp"

#include "map/routing_helpers.hpp"

#include "storage/storage.hpp"

using namespace routing;
using namespace std;

namespace tracking
{
void CmdCppTrack(string const & trackFile, string const & mwmName, string const & user,
                 size_t trackIdx)
{
  storage::Storage storage;
  auto const numMwmIds = CreateNumMwmIds(storage);
  MwmToMatchedTracks mwmToMatchedTracks;
  ReadTracks(numMwmIds, trackFile, mwmToMatchedTracks);

  MatchedTrack const & track =
      GetMatchedTrack(mwmToMatchedTracks, *numMwmIds, mwmName, user, trackIdx);

  cout.precision(numeric_limits<double>::max_digits10);
  for (MatchedTrackPoint const & point : track)
  {
    cout << "  {" << point.GetDataPoint().m_latLon.lat << ", " << point.GetDataPoint().m_latLon.lon
         << "}," << endl;
  }
}
}  // namespace tracking
