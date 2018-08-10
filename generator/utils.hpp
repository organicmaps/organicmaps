#pragma once

#include "generator/gen_mwm_info.hpp"

#include "indexer/data_source.hpp"
#include "indexer/mwm_set.hpp"

#include "platform/local_country_file.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <string>

namespace generator
{
/// \brief This class is wrapper around |DataSource| if only one mwm is registered in DataSource.
class SingleMwmDataSource
{
public:
  /// \param mwmPath is a path to mwm which should be registerd in DataSource.
  explicit SingleMwmDataSource(std::string const & mwmPath);

  DataSource & GetDataSource() { return m_dataSource; }
  std::string GetPath(MapOptions file) const { return m_countryFile.GetPath(file); }
  MwmSet::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  FrozenDataSource m_dataSource;
  platform::LocalCountryFile m_countryFile;
  MwmSet::MwmId m_mwmId;
};

void LoadDataSource(DataSource & dataSource);

template <typename ToDo>
bool ForEachOsmId2FeatureId(std::string const & path, ToDo && toDo)
{
  gen::OsmID2FeatureID mapping;
  try
  {
    FileReader reader(path);
    NonOwningReaderSource source(reader);
    mapping.Read(source);
  }
  catch (FileReader::Exception const & e)
  {
    LOG(LERROR, ("Exception while reading file:", path, ", message:", e.Msg()));
    return false;
  }

  mapping.ForEach([&](gen::OsmID2FeatureID::ValueT const & p) {
    toDo(p.first /* osm id */, p.second /* feature id */);
  });

  return true;
}
}  // namespace generator
