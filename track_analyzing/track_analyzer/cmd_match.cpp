#include "track_analyzing/serialization.hpp"
#include "track_analyzing/track.hpp"
#include "track_analyzing/track_analyzer/utils.hpp"
#include "track_analyzing/track_matcher.hpp"
#include "track_analyzing/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include <memory>
#include <string>

using namespace routing;
using namespace std;
using namespace track_analyzing;

namespace
{
void MatchTracks(MwmToTracks const & mwmToTracks, storage::Storage const & storage,
                 NumMwmIds const & numMwmIds, MwmToMatchedTracks & mwmToMatchedTracks)
{
  my::Timer timer;

  uint64_t tracksCount = 0;
  uint64_t pointsCount = 0;
  uint64_t nonMatchedPointsCount = 0;

  auto processMwm = [&](string const & mwmName, UserToTrack const & userToTrack) {
    auto const countryFile = platform::CountryFile(mwmName);
    auto const mwmId = numMwmIds.GetId(countryFile);
    TrackMatcher matcher(storage, mwmId, countryFile);

    auto & userToMatchedTracks = mwmToMatchedTracks[mwmId];

    for (auto const & it : userToTrack)
    {
      string const & user = it.first;
      auto & matchedTracks = userToMatchedTracks[user];
      try
      {
        matcher.MatchTrack(it.second, matchedTracks);
      }
      catch (RootException const & e)
      {
        LOG(LERROR, ("Can't match track for mwm:", mwmName, ", user:", user));
        LOG(LERROR, ("  ", e.what()));
      }

      if (matchedTracks.empty())
        userToMatchedTracks.erase(user);
    }

    if (userToMatchedTracks.empty())
      mwmToMatchedTracks.erase(mwmId);

    tracksCount += matcher.GetTracksCount();
    pointsCount += matcher.GetPointsCount();
    nonMatchedPointsCount += matcher.GetNonMatchedPointsCount();

    LOG(LINFO, (numMwmIds.GetFile(mwmId).GetName(), ", users:", userToTrack.size(), ", tracks:",
                matcher.GetTracksCount(), ", points:", matcher.GetPointsCount(),
                ", non matched points:", matcher.GetNonMatchedPointsCount()));
  };

  ForTracksSortedByMwmName(mwmToTracks, numMwmIds, processMwm);

  LOG(LINFO,
      ("Matching finished, elapsed:", timer.ElapsedSeconds(), "seconds, tracks:", tracksCount,
       ", points:", pointsCount, ", non matched points:", nonMatchedPointsCount));
}

}  // namespace

namespace track_analyzing
{
void CmdMatch(string const & logFile, string const & trackFile)
{
  LOG(LINFO, ("Matching", logFile));
  shared_ptr<NumMwmIds> numMwmIds;
  storage::Storage storage;
  MwmToTracks mwmToTracks;
  ParseTracks(logFile, numMwmIds, storage, mwmToTracks);

  MwmToMatchedTracks mwmToMatchedTracks;
  MatchTracks(mwmToTracks, storage, *numMwmIds, mwmToMatchedTracks);

  FileWriter writer(trackFile, FileWriter::OP_WRITE_TRUNCATE);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Serialize(mwmToMatchedTracks, writer);
  LOG(LINFO, ("Matched tracks were saved to", trackFile));
}
}  // namespace track_analyzing
