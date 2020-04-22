#pragma once

#include "generator/affiliation.hpp"
#include "generator/cities_boundaries_builder.hpp"
#include "generator/feature_builder.hpp"
#include "generator/place_processor.hpp"
#include "generator/type_helper.hpp"

#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"
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
  PlaceHelper() : m_table(std::make_shared<OsmIdToBoundariesTable>()), m_processor(m_table) {}

  explicit PlaceHelper(std::string const & filename) : PlaceHelper()
  {
    feature::ForEachFeatureRawFormat<feature::serialization_policy::MaxAccuracy>(
        filename, [&](auto const & fb, auto const &) { m_processor.Add(fb); });
  }

  static bool IsPlace(feature::FeatureBuilder const & fb)
  {
    auto const type = GetPlaceType(fb);
    return type != ftype::GetEmptyValue() && !fb.GetName().empty() && NeedProcessPlace(fb);
  }

  bool Process(feature::FeatureBuilder const & fb)
  {
    m_processor.Add(fb);
    return true;
  }

  std::vector<PlaceProcessor::PlaceWithIds> GetFeatures() { return m_processor.ProcessPlaces(); }

  std::shared_ptr<OsmIdToBoundariesTable> GetTable() const { return m_table; }

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
  void ProcessForPromoCatalog(std::vector<PlaceProcessor::PlaceWithIds> & fbs);

  std::string m_citiesFilename;
  std::string m_temporaryMwmPath;
  feature::AffiliationInterface const & m_affiliation;
  PlaceHelper & m_citiesHelper;
  size_t m_threadsCount;
};

template <typename ToDo>
void ForEachCountry(std::string const & temporaryMwmPath, ToDo && toDo)
{
  Platform::FilesList fileList;
  Platform::GetFilesByExt(temporaryMwmPath, DATA_FILE_EXTENSION_TMP, fileList);
  for (auto const & filename : fileList)
    toDo(filename);
}

// Writes |fbs| to countries tmp.mwm files that |fbs| belongs to according to |affiliations|.
template <class SerializationPolicy = feature::serialization_policy::MaxAccuracy>
void AppendToCountries(std::vector<feature::FeatureBuilder> const & fbs,
                       std::vector<std::vector<std::string>> const & affiliations,
                       std::string const & temporaryMwmPath, size_t threadsCount)
{
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
      feature::FeatureBuilderWriter<SerializationPolicy> collector(path, FileWriter::Op::OP_APPEND);
      for (auto const index : indexes)
        collector.Write(fbs[index]);
    });
  }
}

// Sorting for stable features order in final processors.
void Sort(std::vector<feature::FeatureBuilder> & fbs);

std::vector<std::vector<std::string>> GetAffiliations(
    std::vector<feature::FeatureBuilder> const & fbs,
    feature::AffiliationInterface const & affiliation, size_t threadsCount);

std::string GetCountryNameFromTmpMwmPath(std::string filename);

bool FilenameIsCountry(std::string const & filename,
                       feature::AffiliationInterface const & affiliation);
}  // namespace generator
