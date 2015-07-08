#pragma once

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "std/unordered_map.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;


namespace tests
{

class MwmValue : public MwmSet::MwmValueBase
{
};

class TestMwmSet : public MwmSet
{
protected:
  // MwmSet overrides:
  bool GetVersion(LocalCountryFile const & localFile, MwmInfo & info) const override
  {
    int const n = localFile.GetCountryName()[0] - '0';
    info.m_maxScale = n;
    info.m_limitRect = m2::RectD(0, 0, 1, 1);
    info.m_version.format = version::lastFormat;
    return true;
  }

  TMwmValueBasePtr CreateValue(LocalCountryFile const &) const override
  {
    return TMwmValueBasePtr(new MwmValue());
  }

public:
  ~TestMwmSet() override = default;
};
}  // namespace
