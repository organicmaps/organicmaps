#pragma once

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace tests
{

class TestMwmSet : public MwmSet
{
protected:
  /// @name MwmSet overrides
  //@{
  unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override
  {
    int const n = localFile.GetCountryName()[0] - '0';
    unique_ptr<MwmInfo> info(new MwmInfo());
    info->m_maxScale = n;
    info->m_limitRect = m2::RectD(0, 0, 1, 1);
    info->m_version.format = version::lastFormat;
    return info;
  }

  unique_ptr<MwmValueBase> CreateValue(MwmInfo &) const override
  {
    return make_unique<MwmValueBase>();
  }
  //@}
};

}  // namespace
