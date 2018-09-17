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
}
namespace platform
{
class LocalCountryFile;
}
class FeatureBuilder1;

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
  bool Add(FeatureBuilder1 & fb);
  void SetMwmLanguages(std::vector<std::string> const & languages);

  void Finish();

private:
  platform::LocalCountryFile & m_file;
  feature::DataHeader::MapType m_type;
  std::vector<std::string> m_languages;
  std::unique_ptr<feature::FeaturesCollector> m_collector;
  TestIdToBoundariesTable m_boundariesTable;
  uint32_t m_version = 0;
};
}  // namespace tests_support
}  // namespace generator
