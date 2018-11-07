#include "track_analyzing/track.hpp"
#include "track_analyzing/track_analyzer/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "base/logging.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace track_analyzing
{
using namespace routing;
using namespace std;

void CmdUnmatchedTracks(string const & logFile, string const & trackFileCsv)
{
  LOG(LINFO, ("Saving unmatched tracks", logFile));
  storage::Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);
  MwmToTracks mwmToTracks;
  ParseTracks(logFile, numMwmIds, mwmToTracks);

  string const sep = ",";
  ofstream ofs(trackFileCsv, std::ofstream::out);
  for (auto const & kv : mwmToTracks)
  {
    for (auto const & idTrack : kv.second)
    {
      ofs << numMwmIds->GetFile(kv.first).GetName() << sep << idTrack.first;
      for (auto const & pnt : idTrack.second)
        ofs << sep << pnt.m_timestamp << sep << pnt.m_latLon.lat << sep << pnt.m_latLon.lon;
      ofs << "\n";
    }
  }
}
}  // namespace track_analyzing
