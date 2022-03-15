#pragma once

#include "generator/feature_builder.hpp"
#include "generator/generate_info.hpp"

namespace generator
{
namespace tests_support
{

bool MakeFakeBordersFile(std::string const & intemediatePath, std::string const & fileName);

class TestRawGenerator
{
  feature::GenerateInfo m_genInfo;
  std::string const & GetTmpPath() const { return m_genInfo.m_cacheDir; }

public:
  TestRawGenerator();
  ~TestRawGenerator();

  void SetupTmpFolder(std::string const & tmpPath);

  void BuildFB(std::string const & osmFilePath, std::string const & mwmName);
  void BuildFeatures(std::string const & mwmName);

  template <class FnT> void ForEachFB(std::string const & mwmName, FnT && fn)
  {
    using namespace feature;
    ForEachFeatureRawFormat(m_genInfo.GetTmpFileName(mwmName), [&fn](FeatureBuilder const & fb, uint64_t)
    {
      fn(fb);
    });
  }

  std::string GetMwmPath(std::string const & mwmName) const;
  feature::GenerateInfo const & GetGenInfo() const { return m_genInfo; }
};

} // namespace tests_support
} // namespace generator
