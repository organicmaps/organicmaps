#pragma once

#include "generator/cities_boundaries_builder.hpp"

#include "indexer/data_header.hpp"

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
  TestMwmBuilder(platform::LocalCountryFile & file, feature::DataHeader::MapType type);

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
};
}  // namespace tests_support
}  // namespace generator
