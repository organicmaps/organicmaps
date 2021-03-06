#pragma once

#include "generator/affiliation.hpp"
#include "generator/cities_boundaries_builder.hpp"
#include "generator/feature_builder.hpp"
#include "generator/place_processor.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"
#include "base/thread_pool_computational.hpp"

#include "defines.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace generator
{
class PlaceHelper
{
public:
  PlaceHelper();
  explicit PlaceHelper(std::string const & filename);

  static bool IsPlace(feature::FeatureBuilder const & fb);

  bool Process(feature::FeatureBuilder const & fb);
  std::vector<PlaceProcessor::PlaceWithIds> GetFeatures();
  std::shared_ptr<OsmIdToBoundariesTable> GetTable() const;

private:
  std::shared_ptr<OsmIdToBoundariesTable> m_table;
  PlaceProcessor m_processor;
};

class ProcessorCities
{
public:
  ProcessorCities(std::string const & temporaryMwmPath,
                  feature::AffiliationInterface const & affiliation, PlaceHelper & citiesHelper,
                  size_t threadsCount = 1);

  void SetPromoCatalog(std::string const & filename);

  void Process();

private:
  void ProcessForPromoCatalog(std::vector<PlaceProcessor::PlaceWithIds> & fbsWithIds);

  std::string m_citiesFilename;
  std::string m_temporaryMwmPath;
  feature::AffiliationInterface const & m_affiliation;
  PlaceHelper & m_citiesHelper;
  size_t m_threadsCount;
};

template <typename ToDo>
void ForEachMwmTmp(std::string const & temporaryMwmPath, ToDo && toDo, size_t threadsCount = 1)
{
  Platform::FilesList fileList;
  Platform::GetFilesByExt(temporaryMwmPath, DATA_FILE_EXTENSION_TMP, fileList);
  base::thread_pool::computational::ThreadPool pool(threadsCount);
  for (auto const & filename : fileList)
  {
    auto countryName = filename;
    strings::ReplaceLast(countryName, DATA_FILE_EXTENSION_TMP, "");
    pool.SubmitWork(std::forward<ToDo>(toDo), countryName, base::JoinPath(temporaryMwmPath, filename));
  }
}

std::vector<std::vector<std::string>> GetAffiliations(
    std::vector<feature::FeatureBuilder> const & fbs,
    feature::AffiliationInterface const & affiliation, size_t threadsCount);

// Writes |fbs| to countries mwm.tmp files. Returns affiliations - country matches for |fbs|.
template <class SerializationPolicy = feature::serialization_policy::MaxAccuracy>
std::vector<std::vector<std::string>> AppendToMwmTmp(std::vector<feature::FeatureBuilder> const & fbs,
                       feature::AffiliationInterface const & affiliation,
                       std::string const & temporaryMwmPath, size_t threadsCount = 1)
{
  auto const affiliations = GetAffiliations(fbs, affiliation, threadsCount);
  std::unordered_map<std::string, std::vector<size_t>> countryToFbsIndexes;
  for (size_t i = 0; i < fbs.size(); ++i)
  {
    for (auto const & country : affiliations[i])
      countryToFbsIndexes[country].emplace_back(i);
  }

  base::thread_pool::computational::ThreadPool pool(threadsCount);
  for (auto && p : countryToFbsIndexes)
  {
    pool.SubmitWork([&, country{std::move(p.first)}, indexes{std::move(p.second)}]() {
      auto const path = base::JoinPath(temporaryMwmPath, country + DATA_FILE_EXTENSION_TMP);
      feature::FeatureBuilderWriter<SerializationPolicy> collector(
            path, FileWriter::Op::OP_APPEND);
      for (auto const index : indexes)
        collector.Write(fbs[index]);
    });
  }

  return affiliations;
}

bool Less(feature::FeatureBuilder const & lhs, feature::FeatureBuilder const & rhs);

// Ordering for stable features order in final processors.
void Order(std::vector<feature::FeatureBuilder> & fbs);

void OrderTextFileByLine(std::string const & filename);
}  // namespace generator
