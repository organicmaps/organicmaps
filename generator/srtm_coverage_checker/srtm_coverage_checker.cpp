#include "generator/srtm_parser.hpp"

#include "map/feature_vec_model.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "indexer/feature_altitude.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "base/logging.hpp"

#include <algorithm>

#include "3party/gflags/src/gflags/gflags.h"

DEFINE_string(srtm_path, "", "Path to directory with SRTM files");
DEFINE_string(mwm_path, "", "Path to mwm files (writable dir)");

// TODO: remove when routing_test_tools becomes a library.
namespace
{
void ChangeMaxNumberOfOpenFiles(size_t n)
{
  struct rlimit rlp;
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = n;
  setrlimit(RLIMIT_NOFILE, &rlp);
}

shared_ptr<model::FeaturesFetcher> CreateFeaturesFetcher(vector<LocalCountryFile> const & localFiles)
{
  size_t const maxOpenFileNumber = 1024;
  ChangeMaxNumberOfOpenFiles(maxOpenFileNumber);
  shared_ptr<model::FeaturesFetcher> featuresFetcher(new model::FeaturesFetcher);
  featuresFetcher->InitClassificator();

  for (LocalCountryFile const & localFile : localFiles)
  {
    auto p = featuresFetcher->RegisterMap(localFile);
    if (p.second != MwmSet::RegResult::Success)
    {
      ASSERT(false, ("Can't register", localFile));
      return nullptr;
    }
  }
  return featuresFetcher;
}
}  // namespace

int main(int argc, char * argv[])
{
  google::SetUsageMessage("SRTM coverage checker.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  Platform & platform = GetPlatform();
  if (!FLAGS_mwm_path.empty())
    platform.SetWritableDirForTests(FLAGS_mwm_path);

  if (FLAGS_srtm_path.empty())
  {
    LOG(LERROR, ("SRTM files directory is not specified."));
    return -1;
  }

  LOG(LINFO, ("writable dir =", platform.WritableDir()));
  LOG(LINFO, ("srtm dir =", FLAGS_srtm_path));

  vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                       localFiles);

  auto fetcher = CreateFeaturesFetcher(localFiles);
  generator::SrtmTileManager manager(FLAGS_srtm_path);

  for (auto & file : localFiles)
  {
    file.SyncWithDisk();
    if (file.GetFiles() != MapOptions::MapWithCarRouting)
    {
      LOG(LINFO, ("Warning! Routing file not found for:", file.GetCountryName()));
      continue;
    }

    FilesMappingContainer container(file.GetPath(MapOptions::CarRouting));
    if (!container.IsExist(ROUTING_FTSEG_FILE_TAG))
    {
      LOG(LINFO, ("Warning! Mwm file has not routing ftseg section:", file.GetCountryName()));
      continue;
    }

    routing::TDataFacade dataFacade;
    dataFacade.Load(container);

    OsrmFtSegMapping segMapping;
    segMapping.Load(container, file);
    segMapping.Map(container);

    size_t all = 0;
    size_t good = 0;

    for (TOsrmNodeId i = 0; i < dataFacade.GetNumberOfNodes(); ++i)
    {
      buffer_vector<OsrmMappingTypes::FtSeg, 8> buffer;
      segMapping.ForEachFtSeg(i, MakeBackInsertFunctor(buffer));

      vector<m2::PointD> path;
      for (size_t k = 0; k < buffer.size(); ++k)
      {
        auto const & segment = buffer[k];
        if (!segment.IsValid())
          continue;
        // Load data from drive.
        FeatureType ft;
        Index::FeaturesLoaderGuard loader(
            fetcher->GetIndex(), fetcher->GetIndex().GetMwmIdByCountryFile(file.GetCountryFile()));
        if (!loader.GetFeatureByIndex(segment.m_fid, ft))
          continue;
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        // Get points in proper direction.
        auto const startIdx = segment.m_pointStart;
        auto const endIdx = segment.m_pointEnd;
        for (auto idx = std::min(startIdx, endIdx); idx <= std::max(startIdx, endIdx); ++idx)
          path.push_back(ft.GetPoint(idx));

        all += path.size();
        for (auto const & point : path)
        {
          auto const height = manager.GetHeight(MercatorBounds::ToLatLon(point));
          if (height != feature::kInvalidAltitude)
            good++;
        }
      }
    }

    auto const bad = all - good;
    auto const percent = all == 0 ? 0.0 : bad * 100.0 / all;
    if (percent > 10.0)
    {
      LOG(LINFO, ("Huge error rate in:", file.GetCountryName(), "good:", good, "bad:", bad, "all:",
                  all, "%:", percent));
    }
  }

  return 0;
}
