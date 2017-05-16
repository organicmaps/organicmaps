#pragma once

#include "track_analyzing/exceptions.hpp"
#include "track_analyzing/track.hpp"

#include "routing/geometry.hpp"
#include "routing/num_mwm_id.hpp"

#include <boost/filesystem.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace tracking
{
double CalcSubtrackLength(MatchedTrack const & track, size_t begin, size_t end,
                          routing::Geometry & geometry);
double CalcTrackLength(MatchedTrack const & track, routing::Geometry & geometry);
double CalcSpeedKMpH(double meters, uint64_t secondsElapsed);
bool IsFiltered(std::string const & argument, std::string const & variable);
void ReadTracks(std::shared_ptr<routing::NumMwmIds> numMwmIds, std::string const & filename,
                MwmToMatchedTracks & mwmToMatchedTracks);
MatchedTrack const & GetMatchedTrack(MwmToMatchedTracks const & mwmToMatchedTracks,
                                     routing::NumMwmIds const & numMwmIds,
                                     std::string const & mwmName, std::string const & user,
                                     size_t trackIdx);

template <typename MwmToTracks, typename ToDo>
void ForTracksSortedByMwmName(ToDo && toDo, MwmToTracks const & mwmToTracks,
                              routing::NumMwmIds const & numMwmIds)
{
  std::vector<std::string> mwmNames;
  mwmNames.reserve(mwmToTracks.size());
  for (auto const & it : mwmToTracks)
    mwmNames.push_back(numMwmIds.GetFile(it.first).GetName());
  sort(mwmNames.begin(), mwmNames.end());

  for (auto const & mwmName : mwmNames)
  {
    auto const mwmId = numMwmIds.GetId(platform::CountryFile(mwmName));
    auto mwmIt = mwmToTracks.find(mwmId);
    CHECK(mwmIt != mwmToTracks.cend(), ());
    toDo(mwmName, mwmIt->second);
  }
}

template <typename ToDo>
void ForEachTrackFile(ToDo && toDo, std::string const & filepath, std::string const & extension,
                      shared_ptr<routing::NumMwmIds> numMwmIds)
{
  if (!boost::filesystem::exists(filepath.c_str()))
    MYTHROW(MessageException, ("File doesn't exist", filepath));

  if (boost::filesystem::is_regular(filepath))
  {
    MwmToMatchedTracks mwmToMatchedTracks;
    ReadTracks(numMwmIds, filepath, mwmToMatchedTracks);
    toDo(filepath, mwmToMatchedTracks);
    return;
  }

  if (boost::filesystem::is_directory(filepath))
  {
    for (boost::filesystem::recursive_directory_iterator it(filepath.c_str()), end; it != end; ++it)
    {
      boost::filesystem::path const & path = it->path();
      if (!boost::filesystem::is_regular(path))
        continue;

      if (path.extension() != extension)
        continue;

      MwmToMatchedTracks mwmToMatchedTracks;
      string const & filename = path.string();
      ReadTracks(numMwmIds, filename, mwmToMatchedTracks);
      toDo(filename, mwmToMatchedTracks);
    }

    return;
  }

  MYTHROW(MessageException, (filepath, "is neither a regular file nor a directory't exist"));
}
}  // namespace tracking
