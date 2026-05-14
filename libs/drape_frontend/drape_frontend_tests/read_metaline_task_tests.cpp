#include "testing/testing.hpp"

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/read_metaline_task.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "coding/files_container.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/file_name_utils.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
class TestMwmInfo : public MwmInfo
{
public:
  explicit TestMwmInfo(std::string const & mwmPath)
  {
    m_file = platform::LocalCountryFile(base::GetDirectory(mwmPath),
                                        platform::CountryFile(base::GetNameFromFullPathWithoutExt(mwmPath)),
                                        1 /* version */);
    m_minScale = 1;
    m_maxScale = 20;
    SetStatus(STATUS_REGISTERED);
  }
};

MwmSet::MwmId MakeMwmId(std::string const & mwmPath)
{
  return MwmSet::MwmId(std::make_shared<TestMwmInfo>(mwmPath));
}

void WriteMetalines(std::string const & mwmPath, std::vector<int32_t> const & featureIds)
{
  FilesContainerW cont(mwmPath);
  auto writer = cont.GetWriter(METALINES_FILE_TAG);

  WriteToSink(writer, df::kMetaLinesSectionVersion);
  WriteVarUint(writer, 1u /* metalines count */);
  WriteVarUint(writer, static_cast<uint32_t>(featureIds.size()));
  for (auto const featureId : featureIds)
    WriteVarInt(writer, featureId);
}

df::MapDataProvider MakeEmptyDataProvider()
{
  auto readIds = [](auto const &, m2::RectD const &, int) {};
  auto readFeatures = [](auto const &, std::vector<FeatureID> const &) {};
  auto getMwmHandleById = [](MwmSet::MwmId const &) { return MwmSet::MwmHandle(); };
  auto isCountryLoadedByName = [](std::string_view) { return true; };
  auto updateCurrentCountry = [](m2::PointD const &, int) {};
  auto readTileBackground = [](df::TileKey const &, dp::BackgroundMode) {};
  auto cancelTileBackgroundReading = [](df::TileKey const &, dp::BackgroundMode) {};

  return df::MapDataProvider(std::move(readIds), std::move(readFeatures), std::move(getMwmHandleById),
                             std::move(isCountryLoadedByName), std::move(updateCurrentCountry),
                             std::move(readTileBackground), std::move(cancelTileBackgroundReading));
}
}  // namespace

UNIT_TEST(ReadMetalineTask_SkipsMwmWhenHandleCannotBeLocked)
{
  std::string const mwmPath = GetPlatform().TmpPathForFile("read_metaline_task_", DATA_FILE_EXTENSION);
  SCOPE_GUARD(_, std::bind(Platform::RemoveFileIfExists, std::cref(mwmPath)));
  WriteMetalines(mwmPath, {1, 2});

  auto model = MakeEmptyDataProvider();
  df::ReadMetalineTask task(model, MakeMwmId(mwmPath));
  task.Run();

  df::MetalineCache cache;
  TEST(!task.UpdateCache(cache), ());
  TEST(cache.empty(), ());
}
