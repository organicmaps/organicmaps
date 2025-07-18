#pragma once

#include "search/engine.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include <memory>
#include <string>

class DataSource;

namespace search
{
struct SearchParams;

namespace tests_support
{
class TestSearchEngine
{
public:
  TestSearchEngine(DataSource & dataSource, Engine::Params const & params, bool mockCountryInfo = false);

  void InitAffiliations();

  void SetLocale(std::string const & locale) { m_engine.SetLocale(locale); }

  void LoadCitiesBoundaries() { m_engine.LoadCitiesBoundaries(); }

  std::weak_ptr<ProcessorHandle> Search(SearchParams const & params);

  storage::CountryInfoGetter & GetCountryInfoGetter() { return *m_infoGetter; }

private:
  storage::Affiliations m_affiliations;
  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  Engine m_engine;
};
}  // namespace tests_support
}  // namespace search
