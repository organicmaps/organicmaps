#include "indexer/altitude_loader.hpp"

#include "indexer/data_source.hpp"

#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include "defines.hpp"

#include <algorithm>

#include "3party/succinct/mapper.hpp"

using namespace std;

namespace
{
template <class TCont>
void LoadAndMap(size_t dataSize, ReaderSource<FilesContainerR::TReader> & src, TCont & cont,
                unique_ptr<CopiedMemoryRegion> & region)
{
  vector<uint8_t> data(dataSize);
  src.Read(data.data(), data.size());
  region = make_unique<CopiedMemoryRegion>(move(data));
  coding::MapVisitor visitor(region->ImmutableData());
  cont.map(visitor);
}
}  // namespace

namespace feature
{
AltitudeLoader::AltitudeLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId)
  : m_handle(dataSource.GetMwmHandleById(mwmId))
{
  if (!m_handle.IsAlive())
    return;

  auto const & mwmValue = *m_handle.GetValue();

  m_countryFileName = mwmValue.GetCountryFileName();

  CHECK_GREATER_OR_EQUAL(mwmValue.GetHeader().GetFormat(), version::Format::v8,
                         ("Unsupported mwm format"));

  if (!mwmValue.m_cont.IsExist(ALTITUDES_FILE_TAG))
    return;

  try
  {
    m_reader = make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(ALTITUDES_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    LoadAndMap(m_header.GetAltitudeAvailabilitySize(), src, m_altitudeAvailability,
               m_altitudeAvailabilityRegion);
    LoadAndMap(m_header.GetFeatureTableSize(), src, m_featureTable, m_featureTableRegion);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR, ("File", m_countryFileName, "Error while reading", ALTITUDES_FILE_TAG, "section.", e.Msg()));
  }
}

bool AltitudeLoader::HasAltitudes() const
{
  return m_reader != nullptr && m_header.m_minAltitude != geometry::kInvalidAltitude;
}

geometry::Altitudes const & AltitudeLoader::GetAltitudes(uint32_t featureId, size_t pointCount)
{
  if (!HasAltitudes())
  {
    // There's no altitude section in mwm.
    return m_cache
        .insert(
            make_pair(featureId, geometry::Altitudes(pointCount, geometry::kDefaultAltitudeMeters)))
        .first->second;
  }

  auto const it = m_cache.find(featureId);
  if (it != m_cache.end())
    return it->second;

  if (!m_altitudeAvailability[featureId])
  {
    return m_cache
        .insert(make_pair(featureId, geometry::Altitudes(pointCount, m_header.m_minAltitude)))
        .first->second;
  }

  uint64_t const r = m_altitudeAvailability.rank(featureId);
  CHECK_LESS(r, m_altitudeAvailability.size(), ("Feature Id", featureId, "of", m_countryFileName));
  uint64_t const offset = m_featureTable.select(r);
  CHECK_LESS_OR_EQUAL(offset, m_featureTable.size(), ("Feature Id", featureId, "of", m_countryFileName));

  uint64_t const altitudeInfoOffsetInSection = m_header.m_altitudesOffset + offset;
  CHECK_LESS(altitudeInfoOffsetInSection, m_reader->Size(), ("Feature Id", featureId, "of", m_countryFileName));

  try
  {
    Altitudes altitudes;
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    src.Skip(altitudeInfoOffsetInSection);
    bool const isDeserialized = altitudes.Deserialize(m_header.m_minAltitude, pointCount,
                                                      m_countryFileName, featureId,  src);

    bool const allValid =
        isDeserialized &&
        none_of(altitudes.m_altitudes.begin(), altitudes.m_altitudes.end(),
                [](geometry::Altitude a) { return a == geometry::kInvalidAltitude; });
    if (!allValid)
    {
      LOG(LERROR, ("Only a part point of a feature has a valid altitdue. Altitudes: ", altitudes.m_altitudes,
                   ". Feature Id", featureId, "of", m_countryFileName));
      return m_cache
          .insert(make_pair(featureId, geometry::Altitudes(pointCount, m_header.m_minAltitude)))
          .first->second;
    }

    return m_cache.insert(make_pair(featureId, move(altitudes.m_altitudes))).first->second;
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Feature Id", featureId, "of", m_countryFileName, ". Error while getting altitude data:", e.Msg()));
    return m_cache
        .insert(make_pair(featureId, geometry::Altitudes(pointCount, m_header.m_minAltitude)))
        .first->second;
  }
}
}  // namespace feature
