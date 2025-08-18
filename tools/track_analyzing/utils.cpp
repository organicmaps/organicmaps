#include "track_analyzing/utils.hpp"

#include "track_analyzing/serialization.hpp"

#include "routing/segment.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

#include <cstdint>

using namespace routing;
using namespace std;

namespace track_analyzing
{
double CalcSubtrackLength(MatchedTrack::const_iterator begin, MatchedTrack::const_iterator end, Geometry & geometry)
{
  double length = 0.0;

  Segment prevSegment;
  for (auto it = begin; it != end; ++it)
  {
    MatchedTrackPoint const & point = *it;
    Segment const & segment = point.GetSegment();
    if (segment != prevSegment)
    {
      length += ms::DistanceOnEarth(geometry.GetPoint(segment.GetRoadPoint(false /* front */)),
                                    geometry.GetPoint(segment.GetRoadPoint(true /* front */)));
      prevSegment = segment;
    }
  }

  return length;
}

double CalcTrackLength(MatchedTrack const & track, Geometry & geometry)
{
  return CalcSubtrackLength(track.begin(), track.end(), geometry);
}

double CalcSpeedKMpH(double meters, uint64_t secondsElapsed)
{
  CHECK_GREATER(secondsElapsed, 0, ());
  return measurement_utils::MpsToKmph(meters / static_cast<double>(secondsElapsed));
}

void ReadTracks(shared_ptr<NumMwmIds> numMwmIds, string const & filename, MwmToMatchedTracks & mwmToMatchedTracks)
{
  FileReader reader(filename);
  ReaderSource<FileReader> src(reader);
  MwmToMatchedTracksSerializer serializer(numMwmIds);
  serializer.Deserialize(mwmToMatchedTracks, src);
}

MatchedTrack const & GetMatchedTrack(MwmToMatchedTracks const & mwmToMatchedTracks, NumMwmIds const & numMwmIds,
                                     string const & mwmName, string const & user, size_t trackIdx)
{
  auto const countryFile = platform::CountryFile(mwmName);
  if (!numMwmIds.ContainsFile(countryFile))
    MYTHROW(MessageException, ("Invalid mwm name", mwmName));

  NumMwmId const numMwmId = numMwmIds.GetId(countryFile);

  auto mIt = mwmToMatchedTracks.find(numMwmId);
  if (mIt == mwmToMatchedTracks.cend())
    MYTHROW(MessageException, ("There are no tracks for mwm", mwmName));

  UserToMatchedTracks const & userToMatchedTracks = mIt->second;

  auto uIt = userToMatchedTracks.find(user);
  if (uIt == userToMatchedTracks.end())
    MYTHROW(MessageException, ("There is no user", user));

  vector<MatchedTrack> const & tracks = uIt->second;

  if (trackIdx >= tracks.size())
  {
    MYTHROW(MessageException,
            ("There is no track", trackIdx, "for user", user, ", she has", tracks.size(), "tracks only"));
  }

  return tracks[trackIdx];
}

std::string GetCurrentVersionMwmFile(storage::Storage const & storage, std::string const & mwmName)
{
  return storage.GetFilePath(mwmName, MapFileType::Map);
}

void ForEachTrackFile(std::string const & filepath, std::string const & extension,
                      shared_ptr<routing::NumMwmIds> numMwmIds,
                      std::function<void(std::string const & filename, MwmToMatchedTracks const &)> && toDo)
{
  Platform::EFileType fileType = Platform::EFileType::Unknown;
  Platform::EError const result = Platform::GetFileType(filepath, fileType);

  if (result == Platform::ERR_FILE_DOES_NOT_EXIST)
    MYTHROW(MessageException, ("File doesn't exist", filepath));

  if (result != Platform::ERR_OK)
    MYTHROW(MessageException, ("Can't get file type for", filepath, "result:", result));

  if (fileType == Platform::EFileType::Regular)
  {
    MwmToMatchedTracks mwmToMatchedTracks;
    ReadTracks(numMwmIds, filepath, mwmToMatchedTracks);
    toDo(filepath, mwmToMatchedTracks);
    return;
  }

  if (fileType == Platform::EFileType::Directory)
  {
    Platform::FilesList filesList;
    Platform::GetFilesRecursively(filepath, filesList);

    for (string const & file : filesList)
    {
      if (base::GetFileExtension(file) != extension)
        continue;

      MwmToMatchedTracks mwmToMatchedTracks;
      ReadTracks(numMwmIds, file, mwmToMatchedTracks);
      toDo(file, mwmToMatchedTracks);
    }

    return;
  }

  MYTHROW(MessageException, (filepath, "is neither a regular file nor a directory't exist"));
}
}  // namespace track_analyzing
