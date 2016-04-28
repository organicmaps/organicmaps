#pragma once

#include "indexer/data_header.hpp"

#include "std/unique_ptr.hpp"

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

  void Finish();

private:
  platform::LocalCountryFile & m_file;
  feature::DataHeader::MapType m_type;
  unique_ptr<feature::FeaturesCollector> m_collector;
};
}  // namespace tests_support
}  // namespace generator
