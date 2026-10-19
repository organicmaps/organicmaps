#pragma once

#include "storage/country_info_getter.hpp"

#include "coding/files_container.hpp"

#include <memory>

namespace lightweight
{
/// Used in tests only for now.
class CountryInfoReader : protected storage::CountryInfoGetterBase
{
public:
  CountryInfoReader();

protected:
  // storage::CountryInfoGetterBase overrides:
  bool BelongsToRegion(m2::PointD const & pt, size_t id) const override;

private:
  std::unique_ptr<FilesContainerR> m_reader;
};
}  // namespace lightweight
