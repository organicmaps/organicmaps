#pragma once

#include "generator/cities_boundaries_builder.hpp"

#include "indexer/data_header.hpp"

#include "base/timer.hpp"

#include <memory>
#include <string>
#include <vector>

namespace feature
{
class FeaturesCollector;
class FeatureBuilder;
}
namespace platform
{
class LocalCountryFile;
}
namespace storage
{
class CountryInfoGetter;
}
namespace generator
{
namespace tests_support
{
class TestFeature;

class TestMwmBuilder
{
public:
  TestMwmBuilder(platform::LocalCountryFile & file, feature::DataHeader::MapType type,
                 uint32_t version = base::GenerateYYMMDD(base::SecondsSinceEpoch()));

  ~TestMwmBuilder();

  void Add(TestFeature const & feature);
  void AddSafe(TestFeature const & feature);
  bool Add(feature::FeatureBuilder & fb);

  void SetUKPostcodesData(std::string const & postcodesPath,
                          std::shared_ptr<storage::CountryInfoGetter> const & countryInfoGetter);
  void SetMwmLanguages(std::vector<std::string> const & languages);

  void Finish();

private:
  platform::LocalCountryFile & m_file;
  feature::DataHeader::MapType m_type;
  std::vector<std::string> m_languages;
  std::unique_ptr<feature::FeaturesCollector> m_collector;
  TestIdToBoundariesTable m_boundariesTable;
  std::shared_ptr<storage::CountryInfoGetter> m_postcodesCountryInfoGetter;
  std::string m_ukPostcodesPath;
  uint32_t m_version = 0;
};
}  // namespace tests_support
}  // namespace generator
