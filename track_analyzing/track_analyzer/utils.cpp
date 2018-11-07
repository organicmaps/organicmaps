#include "track_analyzing/track_analyzer/utils.hpp"

#include "track_analyzing/log_parser.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "geometry/tree4d.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

namespace track_analyzing
{
using namespace routing;
using namespace std;
using namespace storage;

void ParseTracks(string const & logFile, shared_ptr<NumMwmIds> const & numMwmIds,
                 MwmToTracks & mwmToTracks)
{
  Platform const & platform = GetPlatform();
  string const dataDir = platform.WritableDir();
  unique_ptr<CountryInfoGetter> countryInfoGetter =
      CountryInfoReader::CreateCountryInfoReader(platform);
  unique_ptr<m4::Tree<NumMwmId>> mwmTree = MakeNumMwmTree(*numMwmIds, *countryInfoGetter);

  LOG(LINFO, ("Parsing", logFile));
  LogParser parser(numMwmIds, move(mwmTree), dataDir);
  parser.Parse(logFile, mwmToTracks);
}
}  // namespace track_analyzing
