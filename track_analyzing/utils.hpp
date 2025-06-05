#pragma once

#include "track_analyzing/exceptions.hpp"
#include "track_analyzing/track.hpp"

#include "routing/geometry.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/storage.hpp"

#include "platform/platform.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace track_analyzing
{
using StringFilter = std::function<bool(std::string const &)>;

double CalcSubtrackLength(MatchedTrack::const_iterator begin, MatchedTrack::const_iterator end,
                          routing::Geometry & geometry);
double CalcTrackLength(MatchedTrack const & track, routing::Geometry & geometry);
double CalcSpeedKMpH(double meters, uint64_t secondsElapsed);
void ReadTracks(std::shared_ptr<routing::NumMwmIds> numMwmIds, std::string const & filename,
                MwmToMatchedTracks & mwmToMatchedTracks);
MatchedTrack const & GetMatchedTrack(MwmToMatchedTracks const & mwmToMatchedTracks,
                                     routing::NumMwmIds const & numMwmIds, std::string const & mwmName,
                                     std::string const & user, size_t trackIdx);
std::string GetCurrentVersionMwmFile(storage::Storage const & storage, std::string const & mwmName);

template <typename MwmToTracks, typename ToDo>
void ForTracksSortedByMwmName(MwmToTracks const & mwmToTracks, routing::NumMwmIds const & numMwmIds, ToDo && toDo)
{
  std::vector<std::string> mwmNames;
  mwmNames.reserve(mwmToTracks.size());
  for (auto const & it : mwmToTracks)
    mwmNames.push_back(numMwmIds.GetFile(it.first).GetName());
  std::sort(mwmNames.begin(), mwmNames.end());

  for (auto const & mwmName : mwmNames)
  {
    auto const mwmId = numMwmIds.GetId(platform::CountryFile(mwmName));
    auto mwmIt = mwmToTracks.find(mwmId);
    CHECK(mwmIt != mwmToTracks.cend(), ());
    toDo(mwmName, mwmIt->second);
  }
}

void ForEachTrackFile(std::string const & filepath, std::string const & extension,
                      std::shared_ptr<routing::NumMwmIds> numMwmIds,
                      std::function<void(std::string const & filename, MwmToMatchedTracks const &)> && toDo);
}  // namespace track_analyzing
