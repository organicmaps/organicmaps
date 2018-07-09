#pragma once

#include "storage/index.hpp"
#include "storage/country_info_getter.hpp"
#include "storage/country_name_getter.hpp"

#include "coding/file_container.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>

namespace lightweight
{
// Protected inheritance for test purposes only.
class CountryInfoReader : protected storage::CountryInfoGetterBase
{
public:
  struct Info
  {
    storage::TCountryId m_id;
    std::string m_name;
  };

  CountryInfoReader();
  Info GetMwmInfo(m2::PointD const & pt) const;

protected:
  bool IsBelongToRegionImpl(size_t id, m2::PointD const & pt) const override;

private:
  std::unique_ptr<FilesContainerR> m_reader;
  storage::CountryNameGetter m_nameGetter;
};
}  // namespace lightweight
