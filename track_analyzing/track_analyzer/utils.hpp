#pragma once

#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "track_analyzing/track.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <track_analyzing/track.hpp>

namespace track_analyzing
{
struct Stats
{
  using NameToCountMapping = std::map<std::string, uint32_t>;

  void Add(Stats const & stats);
  bool operator==(Stats const & stats) const;

  /// \note These fields may present mapping from territory name to either DataPoints
  /// or MatchedTrackPoint count.
  NameToCountMapping m_mwmToTotalDataPoints;
  NameToCountMapping m_countryToTotalDataPoints;
};

/// \brief Parses tracks from |logFile| and fills |mwmToTracks|.
void ParseTracks(std::string const & logFile, std::shared_ptr<routing::NumMwmIds> const & numMwmIds,
                 MwmToTracks & mwmToTracks);

/// \brief Fills |stat| according to |mwmToTracks|.
void AddStat(MwmToTracks const & mwmToTracks, routing::NumMwmIds const & numMwmIds,
             storage::Storage const & storage, Stats & stats);

std::string DebugPrint(Stats const & s);
}  // namespace track_analyzing
