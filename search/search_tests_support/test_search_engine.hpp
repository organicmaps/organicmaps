#pragma once

#include "search/engine.hpp"

#include "storage/country_info_getter.hpp"

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
  TestSearchEngine(DataSource & dataSource, std::unique_ptr<storage::CountryInfoGetter> infoGetter,
                   Engine::Params const & params);
  TestSearchEngine(DataSource & dataSource, Engine::Params const & params);

  void SetLocale(std::string const & locale) { m_engine.SetLocale(locale); }

  void LoadCitiesBoundaries() { m_engine.LoadCitiesBoundaries(); }

  std::weak_ptr<search::ProcessorHandle> Search(search::SearchParams const & params);

  storage::CountryInfoGetter & GetCountryInfoGetter() { return *m_infoGetter; }

private:
  std::unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  search::Engine m_engine;
};
}  // namespace tests_support
}  // namespace search
