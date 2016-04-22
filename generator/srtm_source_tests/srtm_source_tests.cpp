#include "testing/testing.hpp"

#include "generator/srtm_parser.hpp"

#include "map/feature_vec_model.hpp"

#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file_utils.hpp"

#include "coding/file_name_utils.hpp"

namespace
{
#define SRTM_SOURCES_PATH "srtm/e4ftl01.cr.usgs.gov/SRTM/SRTMGL1.003/2000.02.11/"

UNIT_TEST(SRTMCoverageChecker)
{
  vector<platform::LocalCountryFile> localFiles;
  platform::FindAllLocalMapsAndCleanup(numeric_limits<int64_t>::max() /* latestVersion */,
                                       localFiles);

  shared_ptr<model::FeaturesFetcher> fetcher = integration::CreateFeaturesFetcher(localFiles);

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

    Platform & platform = GetPlatform();
    SrtmFileManager manager(my::JoinFoldersToPath(platform.WritableDir(), SRTM_SOURCES_PATH));
    size_t all = 0;
    size_t good = 0;

    for (size_t i = 0; i < dataFacade.GetNumberOfNodes(); ++i)
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
        loader.GetFeatureByIndex(segment.m_fid, ft);
        ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

        // Get points in proper direction.
        auto startIdx = segment.m_pointStart;
        auto endIdx = segment.m_pointEnd;
        if (startIdx < endIdx)
        {
          for (auto idx = startIdx; idx <= endIdx; ++idx)
            path.push_back(ft.GetPoint(idx));
        }
        else
        {
          // I use big signed type because endIdx can be 0.
          for (int64_t idx = startIdx; idx >= static_cast<int64_t>(endIdx); --idx)
            path.push_back(ft.GetPoint(idx));
        }
        for (auto point : path)
        {
          auto height = manager.GetCoordHeight(MercatorBounds::ToLatLon(point));
          all++;
          if (height != kInvalidSRTMHeight)
            good++;
        }
      }
    }
    auto const delta = all - good;
    auto const percent = delta * 100.0 / all;
    if (percent > 10.0)
      LOG(LINFO, ("Huge error rate in:", file.GetCountryName(), "Good:", good, "All:", all,
                  "Delta:", delta, "%:", percent));
  }
}
}  // namespace
