#pragma once

#include <memory>
#include <string>

#include "storage/country.hpp"
#include "storage/storage_defines.hpp"

#include "platform/get_text_by_id.hpp"

namespace search
{
class RegionInfoGetter
{
public:
  void LoadCountriesTree();
  void SetLocale(std::string const & locale);

  std::string GetLocalizedFullName(storage::CountryId const & id) const;
  std::string GetLocalizedCountryName(storage::CountryId const & id) const;

private:
  storage::CountryTree m_countries;
  std::unique_ptr<platform::GetTextById> m_nameGetter;
};
}  // namespace search
