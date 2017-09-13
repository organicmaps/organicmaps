#include "track_analyzing/log_parser.hpp"
#include "track_analyzing/serialization.hpp"
#include "track_analyzing/track.hpp"
#include "track_analyzing/track_matcher.hpp"
#include "track_analyzing/utils.hpp"

#include "map/routing_helpers.hpp"

#include "routing/num_mwm_id.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include "geometry/tree4d.hpp"

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

  storage::Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);

  Platform const & platform = GetPlatform();
  string const dataDir = platform.WritableDir();

  unique_ptr<storage::CountryInfoGetter> countryInfoGetter =
      storage::CountryInfoReader::CreateCountryInfoReader(platform);
  unique_ptr<m4::Tree<NumMwmId>> mwmTree = MakeNumMwmTree(*numMwmIds, *countryInfoGetter);

  LogParser parser(numMwmIds, move(mwmTree), dataDir);
  MwmToTracks mwmToTracks;
  parser.Parse(logFile, mwmToTracks);

  MwmToMatchedTracks mwmToMatchedTracks;
  MatchTracks(mwmToTracks, storage, *numMwmIds, mwmToMatchedTracks);

  FileWriter writer(trackFile, FileWriter::OP_WRITE_TRUNCATE);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Serialize(mwmToMatchedTracks, writer);
  LOG(LINFO, ("Matched tracks were saved to", trackFile));
}
}  // namespace track_analyzing
