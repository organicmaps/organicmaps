#pragma once

#include "track_analyzing/track.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/tree4d.hpp"

#include <memory>
#include <string>

namespace track_analyzing
{
class LogParser final
{
public:
  LogParser(std::shared_ptr<routing::NumMwmIds> numMwmIds, std::unique_ptr<m4::Tree<routing::NumMwmId>> mwmTree,
            std::string const & dataDir);

  void Parse(std::string const & logFile, MwmToTracks & mwmToTracks) const;

private:
  void ParseUserTracks(std::string const & logFile, UserToTrack & userToTrack) const;
  void SplitIntoMwms(UserToTrack const & userToTrack, MwmToTracks & mwmToTracks) const;

  std::shared_ptr<routing::NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<routing::NumMwmId>> m_mwmTree;
  std::string const m_dataDir;
};
}  // namespace track_analyzing
