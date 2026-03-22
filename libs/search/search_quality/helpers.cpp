#include "search/search_quality/helpers.hpp"

#include "indexer/data_source.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "coding/reader.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <fstream>
#include <limits>
#include <utility>

#include "defines.hpp"

#include "cppjansson/cppjansson.hpp"

namespace search
{
namespace search_quality
{
namespace
{
uint64_t ReadVersionFromHeader(platform::LocalCountryFile const & mwm)
{
  std::vector<std::string> const kSpecialFiles = {WORLD_FILE_NAME, WORLD_COASTS_FILE_NAME};
  for (auto const & name : kSpecialFiles)
    if (mwm.GetCountryName() == name)
      return mwm.GetVersion();

  return version::MwmVersion::Read(FilesContainerR(mwm.GetPath(MapFileType::Map))).GetVersion();
}
}  // namespace

void CheckLocale()
{
  std::string const kJson = "{\"coord\":123.456}";
  std::string const kErrorMsg = "Bad locale. Consider setting LC_ALL=C";

  double coord;
  {
    base::Json root(kJson.c_str());
    FromJSONObject(root.get(), "coord", coord);
  }

  std::string line;
  {
    auto root = base::NewJSONObject();
    ToJSONObject(*root, "coord", coord);

    std::unique_ptr<char, JSONFreeDeleter> buffer(json_dumps(root.get(), JSON_COMPACT));

    line.append(buffer.get());
  }

  CHECK_EQUAL(line, kJson, (kErrorMsg));

  {
    std::string const kTest = "123.456";
    double value;
    VERIFY(strings::to_double(kTest, value), (kTest));
    CHECK_EQUAL(strings::to_string(value), kTest, (kErrorMsg));
  }
}

void ReadStringsFromFile(std::string const & path, std::vector<std::string> & result)
{
  std::ifstream stream(path.c_str());
  CHECK(stream.is_open(), ("Can't open", path));

  std::string s;
  while (std::getline(stream, s))
  {
    strings::Trim(s);
    if (!s.empty())
      result.emplace_back(s);
  }
}

void SetPlatformDirs(std::string const & dataPath, std::string const & mwmPath)
{
  Platform & platform = GetPlatform();

  if (!dataPath.empty())
    platform.SetResourceDir(dataPath);

  if (!mwmPath.empty())
    platform.SetWritableDirForTests(mwmPath);

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("resources dir =", platform.ResourcesDir()));
}

void InitViewport(std::string viewportName, m2::RectD & viewport)
{
  std::map<std::string, m2::RectD> const kViewports = {
      {"default", m2::RectD(m2::PointD(0.0, 0.0), m2::PointD(1.0, 1.0))},
      {"moscow", mercator::RectByCenterLatLonAndSizeInMeters(55.7, 37.7, 5000)},
      {"london", mercator::RectByCenterLatLonAndSizeInMeters(51.5, 0.0, 5000)},
      {"zurich", mercator::RectByCenterLatLonAndSizeInMeters(47.4, 8.5, 5000)}};

  auto it = kViewports.find(viewportName);
  if (it == kViewports.end())
  {
    LOG(LINFO, ("Unknown viewport name:", viewportName, "; setting to default"));
    viewportName = "default";
    it = kViewports.find(viewportName);
  }
  CHECK(it != kViewports.end(), ());
  viewport = it->second;
  LOG(LINFO, ("Viewport is set to:", viewportName, DebugPrint(viewport)));
}

void InitDataSource(FrozenDataSource & dataSource, std::string const & mwmListPath)
{
  std::vector<platform::LocalCountryFile> mwms;
  if (!mwmListPath.empty())
  {
    std::vector<std::string> availableMwms;
    ReadStringsFromFile(mwmListPath, availableMwms);
    for (auto const & countryName : availableMwms)
      mwms.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(countryName), 0);
  }
  else
  {
    platform::FindAllLocalMapsAndCleanup(std::numeric_limits<int64_t>::max() /* the latest version */, mwms);
  }

  LOG(LINFO, ("Initializing the data source with the following mwms:"));
  for (auto & mwm : mwms)
  {
    mwm.SyncWithDisk();
    LOG(LINFO, (mwm.GetCountryName(), ReadVersionFromHeader(mwm)));
    dataSource.RegisterMap(mwm);
  }
  LOG(LINFO, ());
}

std::unique_ptr<search::tests_support::TestSearchEngine> InitSearchEngine(DataSource & dataSource,
                                                                          std::string const & locale, size_t numThreads)
{
  search::Engine::Params params;
  params.m_locale = locale;
  params.m_numThreads = base::checked_cast<size_t>(numThreads);

  return std::make_unique<search::tests_support::TestSearchEngine>(dataSource, params);
}
}  // namespace search_quality
}  // namespace search
