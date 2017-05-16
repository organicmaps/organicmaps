#include "track_analyzing/utils.hpp"

#include "track_analyzing/serialization.hpp"

#include "routing/segment.hpp"

#include <cstdint>

using namespace routing;
using namespace std;

namespace tracking
{
double CalcSubtrackLength(MatchedTrack const & track, size_t begin, size_t end, Geometry & geometry)
{
  CHECK_LESS_OR_EQUAL(end, track.size(), ());

  double length = 0.0;

  Segment prevSegment;
  for (size_t i = begin; i < end; ++i)
  {
    MatchedTrackPoint const & point = track[i];
    Segment const & segment = point.GetSegment();
    if (segment != prevSegment)
    {
      length += MercatorBounds::DistanceOnEarth(geometry.GetPoint(segment.GetRoadPoint(false)),
                                                geometry.GetPoint(segment.GetRoadPoint(true)));
      prevSegment = segment;
    }
  }

  return length;
}

double CalcTrackLength(MatchedTrack const & track, Geometry & geometry)
{
  return CalcSubtrackLength(track, 0, track.size(), geometry);
}

double CalcSpeedKMpH(double meters, uint64_t secondsElapsed)
{
  CHECK_GREATER(secondsElapsed, 0, ());
  double constexpr kMPS2KMPH = 60.0 * 60.0 / 1000.0;
  return kMPS2KMPH * meters / static_cast<double>(secondsElapsed);
}

bool IsFiltered(string const & argument, string const & variable)
{
  return !argument.empty() && variable != argument;
}

void ReadTracks(shared_ptr<NumMwmIds> numMwmIds, string const & filename,
                MwmToMatchedTracks & mwmToMatchedTracks)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Deserialize(mwmToMatchedTracks, src);
}

MatchedTrack const & GetMatchedTrack(MwmToMatchedTracks const & mwmToMatchedTracks,
                                     NumMwmIds const & numMwmIds, string const & mwmName,
                                     string const & user, size_t trackIdx)
{
  auto const countryFile = platform::CountryFile(mwmName);
  if (!numMwmIds.ContainsFile(countryFile))
    MYTHROW(MessageException, ("Invalid mwm name", mwmName));

  NumMwmId const numMwmId = numMwmIds.GetId(countryFile);

  auto mIt = mwmToMatchedTracks.find(numMwmId);
  if (mIt == mwmToMatchedTracks.cend())
    MYTHROW(MessageException, ("There is no tracks for mwm", mwmName));

  UserToMatchedTracks const & userToMatchedTracks = mIt->second;

  auto uIt = userToMatchedTracks.find(user);
  if (uIt == userToMatchedTracks.end())
    MYTHROW(MessageException, ("There is no user", user));

  vector<MatchedTrack> const & tracks = uIt->second;

  if (trackIdx >= tracks.size())
  {
    MYTHROW(MessageException, ("There is no track", trackIdx, "for user", user, ", she has",
                               tracks.size(), "tracks only"));
  }

  return tracks[trackIdx];
}
}  // namespace tracking