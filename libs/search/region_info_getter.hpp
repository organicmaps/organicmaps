#pragma once

#include "storage/country_tree.hpp"
#include "storage/storage_defines.hpp"

#include "platform/get_text_by_id.hpp"

#include <memory>
#include <string>

namespace search
{
class RegionInfoGetter
{
public:
  void LoadCountriesTree();
  void SetLocale(std::string const & locale);

  std::string GetLocalizedFullName(storage::CountryId const & id) const;

  using NameBufferT = buffer_vector<std::string, 4>;
  void GetLocalizedFullName(storage::CountryId const & id, NameBufferT & nameParts) const;

  std::string GetLocalizedCountryName(storage::CountryId const & id) const;

private:
  storage::CountryTree m_countries;
  std::unique_ptr<platform::GetTextById> m_nameGetter;
};
}  // namespace search
