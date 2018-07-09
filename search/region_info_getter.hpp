#pragma once

#include <memory>
#include <string>

#include "storage/country.hpp"
#include "storage/index.hpp"

#include "platform/get_text_by_id.hpp"

namespace search
{
class RegionInfoGetter
{
public:
  void LoadCountriesTree();
  void SetLocale(std::string const & locale);

  std::string GetLocalizedFullName(storage::TCountryId const & id) const;
  std::string GetLocalizedCountryName(storage::TCountryId const & id) const;

private:
  storage::TCountryTree m_countries;
  std::unique_ptr<platform::GetTextById> m_nameGetter;
};
}  // namespace search
