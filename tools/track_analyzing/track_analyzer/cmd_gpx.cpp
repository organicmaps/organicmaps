#include "track_analyzing/track.hpp"
#include "track_analyzing/track_analyzer/utils.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>

namespace track_analyzing
{
using namespace routing;
using namespace std;

void CmdGPX(string const & logFile, string const & outputDirName, string const & userID)
{
  if (outputDirName.empty())
  {
    LOG(LERROR, ("Converting to GPX error: the path to the output files is empty"));
    return;
  }

  storage::Storage storage;
  shared_ptr<NumMwmIds> numMwmIds = CreateNumMwmIds(storage);
  MwmToTracks mwmToTracks;
  ParseTracks(logFile, numMwmIds, mwmToTracks);
  for (auto const & kv : mwmToTracks)
  {
    auto const & mwmName = numMwmIds->GetFile(kv.first).GetName();
    size_t i = 0;
    for (auto const & track : kv.second)
    {
      if (!userID.empty() && track.first != userID)
        continue;

      auto const path = base::JoinPath(outputDirName, mwmName + to_string(i) + ".gpx");
      ofstream ofs(path, ofstream::out);
      ofs << "<gpx>\n";
      ofs << "<metadata>\n";
      ofs << "<desc>" << track.first << "</desc>\n";
      ofs << "</metadata>\n";
      for (auto const & point : track.second)
      {
        ofs << "<wpt lat=\"" << point.m_latLon.m_lat << "\" lon=\"" << point.m_latLon.m_lon << "\">"
            << "</wpt>\n";
      }

      ofs << "</gpx>\n";
      if (!userID.empty())
        break;

      ++i;
    }
  }
}
}  // namespace track_analyzing
