#pragma once

#include "storage/storage_defines.hpp"

#include "search/search_tests_support/test_search_engine.hpp"

#include "geometry/rect2d.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class DataSource;
class FrozenDataSource;

namespace search
{
namespace search_quality
{
// todo(@m) We should not need that much.
size_t constexpr kMaxOpenFiles = 4000;

void CheckLocale();

void ReadStringsFromFile(std::string const & path, std::vector<std::string> & result);

void SetPlatformDirs(std::string const & dataPath, std::string const & mwmPath);

void InitViewport(std::string viewportName, m2::RectD & viewport);

void InitDataSource(FrozenDataSource & dataSource, std::string const & mwmListPath);

void InitStorageData(storage::Affiliations & affiliations,
                     storage::CountryNameSynonyms & countryNameSynonyms);

std::unique_ptr<search::tests_support::TestSearchEngine> InitSearchEngine(
    DataSource & dataSource, storage::Affiliations const & affiliations, std::string const & locale,
    size_t numThreads);
}  // namespace search_quality
}  // namespace search
