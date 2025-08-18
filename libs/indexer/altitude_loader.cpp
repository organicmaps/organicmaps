#include "indexer/altitude_loader.hpp"

#include "indexer/mwm_set.hpp"

#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include "defines.hpp"

#include <algorithm>

#include "3party/succinct/mapper.hpp"

namespace feature
{

namespace
{
template <class TCont>
void LoadAndMap(size_t dataSize, ReaderSource<FilesContainerR::TReader> & src, TCont & cont,
                std::unique_ptr<CopiedMemoryRegion> & region)
{
  std::vector<uint8_t> data(dataSize);
  src.Read(data.data(), data.size());
  region = std::make_unique<CopiedMemoryRegion>(std::move(data));
  coding::MapVisitor visitor(region->ImmutableData());
  cont.map(visitor);
}
}  // namespace

AltitudeLoaderBase::AltitudeLoaderBase(MwmValue const & mwmValue)
{
  m_countryFileName = mwmValue.GetCountryFileName();

  if (!mwmValue.m_cont.IsExist(ALTITUDES_FILE_TAG))
    return;

  try
  {
    m_reader = std::make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(ALTITUDES_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    LoadAndMap(m_header.GetAltitudeAvailabilitySize(), src, m_altitudeAvailability, m_altitudeAvailabilityRegion);
    LoadAndMap(m_header.GetFeatureTableSize(), src, m_featureTable, m_featureTableRegion);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR, ("File", m_countryFileName, "Error while reading", ALTITUDES_FILE_TAG, "section.", e.Msg()));
  }
}

bool AltitudeLoaderBase::HasAltitudes() const
{
  return m_reader != nullptr && m_header.m_minAltitude != geometry::kInvalidAltitude;
}

geometry::Altitudes AltitudeLoaderBase::GetAltitudes(uint32_t featureId, size_t pointCount)
{
  if (!HasAltitudes())
  {
    // There's no altitude section in mwm.
    return geometry::Altitudes(pointCount, geometry::kDefaultAltitudeMeters);
  }

  if (!m_altitudeAvailability[featureId])
    return geometry::Altitudes(pointCount, m_header.m_minAltitude);

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
    altitudes.Deserialize(m_header.m_minAltitude, pointCount, m_countryFileName, featureId, src);

    // It's filtered on generator stage.
    ASSERT(none_of(altitudes.m_altitudes.begin(), altitudes.m_altitudes.end(),
                   [](geometry::Altitude a) { return a == geometry::kInvalidAltitude; }),
           (featureId, m_countryFileName));

    return std::move(altitudes.m_altitudes);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Feature Id", featureId, "of", m_countryFileName, ". Error while getting altitude data:", e.Msg()));
    return geometry::Altitudes(pointCount, m_header.m_minAltitude);
  }
}

geometry::Altitudes const & AltitudeLoaderCached::GetAltitudes(uint32_t featureId, size_t pointCount)
{
  auto const it = m_cache.find(featureId);
  if (it != m_cache.end())
    return it->second;

  return m_cache.emplace(featureId, AltitudeLoaderBase::GetAltitudes(featureId, pointCount)).first->second;
}
}  // namespace feature
