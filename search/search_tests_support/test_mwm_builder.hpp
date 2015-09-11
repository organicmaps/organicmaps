#pragma once

#include "indexer/data_header.hpp"

#include "geometry/point2d.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"

class Classificator;

namespace feature
{
class FeaturesCollector;
}
namespace platform
{
class LocalCountryFile;
}

namespace search
{
namespace tests_support
{
class TestMwmBuilder
{
public:
  TestMwmBuilder(platform::LocalCountryFile & file, feature::DataHeader::MapType type);

  ~TestMwmBuilder();

  void AddPOI(m2::PointD const & p, string const & name, string const & lang);

  void AddCity(m2::PointD const & p, string const & name, string const & lang);

  void Finish();

private:
  platform::LocalCountryFile & m_file;
  feature::DataHeader::MapType m_type;
  unique_ptr<feature::FeaturesCollector> m_collector;
  Classificator const & m_classificator;
};
}  // namespace tests_support
}  // namespace search
