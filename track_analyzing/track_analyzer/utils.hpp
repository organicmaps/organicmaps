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
// @TODO Rename to Stats
struct Stat
{
  using NameToCountMapping = std::map<std::string, uint32_t>;

  void Add(Stat const & stat);
  bool operator==(Stat const & stat) const;

  /// \note These fields may present mapping from territory name to either DataPoints
  /// or MatchedTrackPoint count.
  NameToCountMapping m_mwmToTotalDataPoints;
  NameToCountMapping m_countryToTotalDataPoints;
};

/// \brief Parses tracks from |logFile| and fills |numMwmIds|, |storage| and |mwmToTracks|.
void ParseTracks(std::string const & logFile, std::shared_ptr<routing::NumMwmIds> const & numMwmIds,
                 MwmToTracks & mwmToTracks);

/// \brief Fills |stat| according to |mwmToTracks|.
void AddStat(MwmToTracks const & mwmToTracks, routing::NumMwmIds const & numMwmIds,
             storage::Storage const & storage, Stat & stat);

std::string DebugPrint(Stat const & s);
}  // namespace track_analyzing
