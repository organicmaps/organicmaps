#include "testing/testing.hpp"

#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/read_metaline_task.hpp"

#include "indexer/data_source.hpp"

#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

#include "defines.hpp"

#include <string_view>

namespace
{
df::MapDataProvider MakeProvider(df::MapDataProvider::TReadFeaturesFn featureReader,
                                 df::MapDataProvider::TGetMwmHandleFn getMwmHandleFn)
{
  return df::MapDataProvider([](auto const &, m2::RectD const &, int) {}, std::move(featureReader),
                             std::move(getMwmHandleFn), [](std::string_view) { return true; },
                             [](m2::PointD const &, int) {}, [](df::TileKey const &, dp::BackgroundMode) {},
                             [](df::TileKey const &, dp::BackgroundMode) {});
}
}  // namespace

UNIT_TEST(ReadMetalineTask_DeadHandleIsNoOp)
{
  auto model = MakeProvider([](auto const &, std::vector<FeatureID> const &) {},
                            [](MwmSet::MwmId const &) { return MwmSet::MwmHandle{}; });
  df::ReadMetalineTask task(model, MwmSet::MwmId{});
  task.Run();

  df::MetalineCache cache;
  TEST(!task.UpdateCache(cache), ());
  TEST(cache.empty(), ());
}

UNIT_TEST(ReadMetalineTask_PopulatesCacheFromRegisteredMwm)
{
  FrozenDataSource dataSource;
  auto const reg = dataSource.RegisterMap(platform::LocalCountryFile::MakeForTesting("minsk-pass"));
  TEST_EQUAL(reg.second, MwmSet::RegResult::Success, ());

  // Guard against the test going vacuous if minsk-pass.mwm is ever regenerated without metalines.
  {
    auto handle = dataSource.GetMwmHandleById(reg.first);
    TEST(handle.IsAlive(), ());
    TEST(handle.GetValue()->m_cont.IsExist(METALINES_FILE_TAG), ("minsk-pass.mwm has no metalines section"));
  }

  auto model = MakeProvider([&dataSource](auto const & fn, std::vector<FeatureID> const & ids) {
    dataSource.ReadFeatures(fn, ids);
  }, [&dataSource](MwmSet::MwmId const & id) { return dataSource.GetMwmHandleById(id); });
  df::ReadMetalineTask task(model, reg.first);
  task.Run();

  df::MetalineCache cache;
  TEST(task.UpdateCache(cache), ());
  TEST(!cache.empty(), ());
}
