#pragma once

#include "track_analyzing/track.hpp"

#include <string>

namespace track_analyzing
{
class TrackArchiveReader final
{
public:
  void ParseUserTracksFromFile(std::string const & logFile, UserToTrack & userToTrack) const;
};
}  // namespace track_analyzing
