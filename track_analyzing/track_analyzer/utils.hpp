#pragma once

#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "track_analyzing/track.hpp"

#include <memory>
#include <string>
#include <track_analyzing/track.hpp>

namespace track_analyzing
{
/// \brief Parses tracks from |logFile| and fills |numMwmIds|, |storage| and |mwmToTracks|.
void ParseTracks(std::string const & logFile, std::shared_ptr<routing::NumMwmIds> const & numMwmIds,
                 MwmToTracks & mwmToTracks);
}  // namespace track_analyzing
