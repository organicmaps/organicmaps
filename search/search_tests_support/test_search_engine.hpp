#pragma once

#include "indexer/index.hpp"

#include "search/engine.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/weak_ptr.hpp"

class Platform;

namespace storage
{
class CountryInfoGetter;
}

namespace search
{
struct SearchParams;

namespace tests_support
{
class TestSearchEngine : public Index
{
public:
  TestSearchEngine(unique_ptr<storage::CountryInfoGetter> infoGetter,
                   unique_ptr<search::ProcessorFactory> factory, Engine::Params const & params);
  TestSearchEngine(unique_ptr<::search::ProcessorFactory> factory, Engine::Params const & params);
  ~TestSearchEngine() override;

  void SetLocale(string const & locale) { m_engine.SetLocale(locale); }

  void LoadCitiesBoundaries() { m_engine.LoadCitiesBoundaries(); }

  weak_ptr<search::ProcessorHandle> Search(search::SearchParams const & params);

  storage::CountryInfoGetter & GetCountryInfoGetter() { return *m_infoGetter; }

private:
  Platform & m_platform;
  unique_ptr<storage::CountryInfoGetter> m_infoGetter;
  search::Engine m_engine;
};
}  // namespace tests_support
}  // namespace search
