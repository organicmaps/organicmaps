#include "generator/utils.hpp"

#include "search/categories_cache.hpp"
#include "search/localities_source.hpp"
#include "search/mwm_context.hpp"

#include "indexer/feature_processor.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "base/assert.hpp"
#include "base/cancellable.hpp"
#include "base/logging.hpp"

#include <vector>

namespace generator
{
// SingleMwmDataSource -----------------------------------------------------------------------------
SingleMwmDataSource::SingleMwmDataSource(std::string const & mwmPath)
{
  m_countryFile = platform::LocalCountryFile::MakeTemporary(mwmPath);
  m_countryFile.SyncWithDisk();
  CHECK(m_countryFile.OnDisk(MapFileType::Map),
        ("No correct mwm corresponding to local country file:", m_countryFile, ". Path:", mwmPath));

  auto const result = m_dataSource.Register(m_countryFile);
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());
  CHECK(result.first.IsAlive(), ());
  m_mwmId = result.first;
}

void LoadDataSource(DataSource & dataSource)
{
  std::vector<platform::LocalCountryFile> localFiles;

  Platform & platform = GetPlatform();
  platform::FindAllLocalMapsInDirectoryAndCleanup(platform.WritableDir(), 0 /* version */,
                                                  -1 /* latestVersion */, localFiles);
  for (auto const & localFile : localFiles)
  {
    LOG(LINFO, ("Found mwm:", localFile));
    try
    {
      dataSource.RegisterMap(localFile);
    }
    catch (RootException const & ex)
    {
      CHECK(false, (ex.Msg(), "Bad mwm file:", localFile));
    }
  }
}

bool ParseFeatureIdToOsmIdMapping(std::string const & path,
                                  std::unordered_map<uint32_t, base::GeoObjectId> & mapping)
{
  return ForEachOsmId2FeatureId(
      path, [&](generator::CompositeId const & compositeId, uint32_t const featureId) {
        CHECK(mapping.emplace(featureId, compositeId.m_mainId).second,
              ("Several osm ids for feature", featureId, "in file", path));
      });
}

bool ParseFeatureIdToTestIdMapping(std::string const & path,
                                   std::unordered_map<uint32_t, uint64_t> & mapping)
{
  bool success = true;
  feature::ForEachFromDat(path, [&](FeatureType & feature, uint32_t fid) {
    auto const & metatada = feature.GetMetadata();
    auto const testIdStr = metatada.Get(feature::Metadata::FMD_TEST_ID);
    uint64_t testId;
    if (!strings::to_uint64(testIdStr, testId))
    {
      LOG(LERROR, ("Can't parse test id from:", testIdStr, "for the feature", fid));
      success = false;
      return;
    }
    mapping.emplace(fid, testId);
  });
  return success;
}

search::CBV GetLocalities(std::string const & dataPath)
{
  FrozenDataSource dataSource;
  auto const result = dataSource.Register(platform::LocalCountryFile::MakeTemporary(dataPath));
  CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ("Can't register", dataPath));

  search::MwmContext context(dataSource.GetMwmHandleById(result.first));
  base::Cancellable const cancellable;
  return search::CategoriesCache(search::LocalitiesSource{}, cancellable).Get(context);
}
}  // namespace generator
