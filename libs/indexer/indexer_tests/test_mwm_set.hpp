#pragma once

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"

#include "geometry/rect2d.hpp"

#include <memory>

using platform::CountryFile;
using platform::LocalCountryFile;

namespace tests
{

class TestMwmSet : public MwmSet
{
protected:
  /// @name MwmSet overrides
  //@{
  std::unique_ptr<MwmInfo> CreateInfo(platform::LocalCountryFile const & localFile) const override
  {
    int const n = localFile.GetCountryName()[0] - '0';
    auto info = std::make_unique<MwmInfo>();
    info->m_maxScale = n;
    info->m_bordersRect = m2::RectD(0, 0, 1, 1);
    info->m_version.SetFormat(version::Format::lastFormat);
    return info;
  }

  std::unique_ptr<MwmValue> CreateValue(MwmInfo & info) const override
  {
    return std::make_unique<MwmValue>(info.GetLocalFile());
  }
  //@}
};

}  // namespace tests
