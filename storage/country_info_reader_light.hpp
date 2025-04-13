#pragma once

#include "storage/country_info_getter.hpp"
#include "storage/country_name_getter.hpp"
#include "storage/storage_defines.hpp"

#include "coding/files_container.hpp"

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace lightweight
{
class CountryInfoReader : protected storage::CountryInfoGetterBase
{
public:
  struct Info
  {
    storage::CountryId m_id;
    std::string m_name;
  };

  CountryInfoReader();
  /// @note Be careful here, because "lightweight" has no region's geometry cache.
  Info GetMwmInfo(m2::PointD const & pt) const;

protected:
  void LoadRegionsFromDisk(size_t id, std::vector<m2::RegionD> & regions) const;

  // storage::CountryInfoGetterBase overrides:
  bool BelongsToRegion(m2::PointD const & pt, size_t id) const override;

private:
  std::unique_ptr<FilesContainerR> m_reader;
  storage::CountryNameGetter m_nameGetter;
};
}  // namespace lightweight
