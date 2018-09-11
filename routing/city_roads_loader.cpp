#include "routing/city_roads_loader.hpp"

#include "indexer/data_source.hpp"

#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"

#include <utility>

namespace routing
{
CityRoadsLoader::CityRoadsLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId)
{
  MwmSet::MwmHandle m_handle(dataSource.GetMwmHandleById(mwmId));
  if (!m_handle.IsAlive())
    return;

  auto const & mwmValue = *m_handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(CITY_ROADS_FILE_TAG))
    return;

  try
  {
    auto reader = std::make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(CITY_ROADS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*reader);

    CityRoadsHeader header;
    header.Deserialize(src);

    std::vector<uint8_t> data(header.m_dataSize);
    src.Read(data.data(), data.size());
    m_cityRoadsRegion = std::make_unique<CopiedMemoryRegion>(std::move(data));
    coding::MapVisitor visitor(m_cityRoadsRegion->ImmutableData());
    m_cityRoads.map(visitor);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", mwmValue.GetCountryFileName(), "Error while reading", CITY_ROADS_FILE_TAG,
                 "section.", e.Msg()));
  }
}

bool CityRoadsLoader::IsCityRoad(uint32_t fid) const
{
  if (fid < m_cityRoads.size())
    return m_cityRoads[fid];

  return false;
}
}  // namespace routing
