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
using namespace tracking;

namespace
{
void MatchTracks(MwmToTracks const & mwmToTracks, storage::Storage const & storage,
                 NumMwmIds const & numMwmIds, MwmToMatchedTracks & mwmToMatchedTracks)
{
  my::Timer timer;

  size_t tracksCount = 0;
  size_t shortTracksCount = 0;
  size_t pointsCount = 0;
  size_t shortTrackPointsCount = 0;
  size_t nonMatchedPointsCount = 0;

  ForTracksSortedByMwmName(
      [&](string const & mwmName, UserToTrack const & userToTrack) {
        auto const countryFile = platform::CountryFile(mwmName);
        auto const mwmId = numMwmIds.GetId(countryFile);
        TrackMatcher matcher(storage, mwmId, countryFile);

        auto & userToMatchedTracks = mwmToMatchedTracks[mwmId];

        for (auto const & it : userToTrack)
        {
          auto & matchedTracks = userToMatchedTracks[it.first];
          matcher.MatchTrack(it.second, matchedTracks);

          if (matchedTracks.empty())
            userToMatchedTracks.erase(it.first);
        }

        if (userToMatchedTracks.empty())
          mwmToMatchedTracks.erase(mwmId);

        tracksCount += matcher.GetTracksCount();
        shortTracksCount += matcher.GetShortTracksCount();
        pointsCount += matcher.GetPointsCount();
        shortTrackPointsCount += matcher.GetShortTrackPointsCount();
        nonMatchedPointsCount += matcher.GetNonMatchedPointsCount();

        LOG(LINFO, (numMwmIds.GetFile(mwmId).GetName(), ", users:", userToTrack.size(), ", tracks:",
                    matcher.GetTracksCount(), ", short tracks:", matcher.GetShortTracksCount(),
                    ", points:", matcher.GetPointsCount(), ", short track points",
                    matcher.GetShortTrackPointsCount(), ", non matched points:",
                    matcher.GetNonMatchedPointsCount()));
      },
      mwmToTracks, numMwmIds);

  LOG(LINFO,
      ("Matching finished, elapsed:", timer.ElapsedSeconds(), "seconds, tracks:", tracksCount,
       ", short tracks:", shortTracksCount, ", points:", pointsCount, ", short track points",
       shortTrackPointsCount, ", non matched points:", nonMatchedPointsCount));
}

}  // namespace

namespace tracking
{
void CmdMatch(string const & logFile, string const & trackFile)
{
  LOG(LINFO, ("Matching", logFile));

  storage::Storage storage;
  storage.RegisterAllLocalMaps();
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);

  Platform const & platform = GetPlatform();
  string const dataDir = platform.WritableDir();

  unique_ptr<storage::CountryInfoGetter> countryInfoGetter =
      storage::CountryInfoReader::CreateCountryInfoReader(platform);
  unique_ptr<m4::Tree<NumMwmId>> mwmTree = MakeNumMwmTree(*numMwmIds, *countryInfoGetter);

  tracking::LogParser parser(numMwmIds, move(mwmTree), dataDir);
  MwmToTracks mwmToTracks;
  parser.Parse(logFile, mwmToTracks);

  MwmToMatchedTracks mwmToMatchedTracks;
  MatchTracks(mwmToTracks, storage, *numMwmIds, mwmToMatchedTracks);

  FileWriter writer(trackFile, FileWriter::OP_WRITE_TRUNCATE);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Serialize(mwmToMatchedTracks, writer);
  LOG(LINFO, ("Matched track was saved to", trackFile));
}
}  // namespace tracking
