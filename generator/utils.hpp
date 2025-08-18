#pragma once

#include "generator/gen_mwm_info.hpp"
#include "generator/osm_element.hpp"

#include "search/cbv.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <csignal>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#define MAIN_WITH_ERROR_HANDLING(func)             \
  int main(int argc, char ** argv)                 \
  {                                                \
    std::signal(SIGABRT, generator::ErrorHandler); \
    std::signal(SIGSEGV, generator::ErrorHandler); \
    return func(argc, argv);                       \
  }

namespace generator
{
void SetLastError(std::string error);
void ErrorHandler(int signum);

/// \brief This class is wrapper around |DataSource| if only one mwm is registered in DataSource.
class SingleMwmDataSource
{
public:
  /// \param mwmPath is a path to mwm which should be registerd in DataSource.
  explicit SingleMwmDataSource(std::string const & mwmPath);

  DataSource & GetDataSource() { return m_dataSource; }
  platform::LocalCountryFile const & GetLocalCountryFile() const { return m_countryFile; }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }
  MwmSet::MwmHandle GetHandle() { return GetDataSource().GetMwmHandleById(GetMwmId()); }

private:
  FrozenDataSource m_dataSource;
  platform::LocalCountryFile m_countryFile;
  MwmSet::MwmId m_mwmId;
};

class FeatureGetter
{
public:
  explicit FeatureGetter(std::string const & countryFullPath);

  std::unique_ptr<FeatureType> GetFeatureByIndex(uint32_t index) const;

private:
  SingleMwmDataSource m_mwm;
  std::unique_ptr<FeaturesLoaderGuard> m_guard;
};

template <typename ToDo>
bool ForEachOsmId2FeatureId(std::string const & path, ToDo && toDo)
{
  generator::OsmID2FeatureID mapping;
  try
  {
    FileReader reader(path);
    NonOwningReaderSource source(reader);
    mapping.ReadAndCheckHeader(source);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LERROR, ("Exception while reading file:", path, ", message:", e.Msg()));
    return false;
  }

  mapping.ForEach([&](auto const & p) { toDo(p.first /* osm id */, p.second /* feature id */); });
  return true;
}

bool ParseFeatureIdToOsmIdMapping(std::string const & path, std::unordered_map<uint32_t, base::GeoObjectId> & mapping);
bool ParseFeatureIdToTestIdMapping(std::string const & path, std::unordered_map<uint32_t, uint64_t> & mapping);

search::CBV GetLocalities(std::string const & dataPath);

struct MapcssRule
{
  bool Matches(std::vector<OsmElement::Tag> const & tags) const;

  std::vector<OsmElement::Tag> m_tags;
  std::vector<std::string> m_mandatoryKeys;
  std::vector<std::string> m_forbiddenKeys;
};

using TypeStrings = std::vector<std::string>;
using MapcssRules = std::vector<std::pair<TypeStrings, MapcssRule>>;

MapcssRules ParseMapCSS(std::unique_ptr<Reader> reader);

std::ofstream OfstreamWithExceptions(std::string const & name);
}  // namespace generator
