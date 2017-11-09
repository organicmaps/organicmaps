#pragma once

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "platform/local_country_file_utils.hpp"

#include "storage/country_info_getter.hpp"

#include <indexer/index.hpp>

#include <vector>

namespace indexer
{
namespace tests_support
{
class TestMwmEnvironment
{
public:
  using MwmList = std::vector<platform::LocalCountryFile>;

  TestMwmEnvironment();
  ~TestMwmEnvironment();

  template <typename TBuildFn>
  MwmSet::MwmId BuildMwm(string const & name, TBuildFn && fn, int64_t version = 0)
  {
    m_mwmFiles.emplace_back(GetPlatform().WritableDir(), platform::CountryFile(name), version);
    auto & file = m_mwmFiles.back();
    Cleanup(file);

    {
      generator::tests_support::TestMwmBuilder builder(file, feature::DataHeader::country);
      fn(builder);
    }

    auto result = m_index.RegisterMap(file);
    CHECK_EQUAL(result.second, MwmSet::RegResult::Success, ());

    auto const & id = result.first;

    auto const & info = id.GetInfo();
    if (info)
      m_infoGetter.AddCountry(storage::CountryDef(name, info->m_limitRect));

    CHECK(id.IsAlive(), ());

    return id;
  }

  template <typename Checker>
  FeatureID FeatureIdForPoint(m2::PointD const & mercator, Checker const & checker)
  {
    m2::RectD const rect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, 0.2 /* rect width */);
    FeatureID id;
    auto const fn = [&id, &checker](FeatureType const & featureType)
    {
      if (checker(featureType))
        id = featureType.GetID();
    };
    m_index.ForEachInRect(fn, rect, scales::GetUpperScale());
    CHECK(id.IsValid(), ());
    return id;
  }

  void Cleanup(platform::LocalCountryFile const & map);
  bool RemoveMwm(MwmSet::MwmId const & mwmId);

  Index & GetIndex() { return m_index; }
  Index const & GetIndex() const { return m_index; }

  MwmList & GetMwms() { return m_mwmFiles; }
  MwmList const & GetMwms() const { return m_mwmFiles; }

private:
  Index m_index;
  storage::CountryInfoGetterForTesting m_infoGetter;
  MwmList m_mwmFiles;
};
}  // namespace tests_support
}  // namespace indexer