#pragma once

#include "storage/country_tree.hpp"
#include "storage/storage_defines.hpp"

#include "platform/get_text_by_id.hpp"

#include <memory>
#include <string>
#include <vector>

namespace search
{
class RegionInfoGetter
{
public:
  void LoadCountriesTree();
  void SetLocale(std::string const & locale);

  std::string GetLocalizedFullName(storage::CountryId const & id) const;
  void GetLocalizedFullName(storage::CountryId const & id,
                            std::vector<std::string> & nameParts) const;
  std::string GetLocalizedCountryName(storage::CountryId const & id) const;

private:
  storage::CountryTree m_countries;
  std::unique_ptr<platform::GetTextById> m_nameGetter;
};
}  // namespace search
