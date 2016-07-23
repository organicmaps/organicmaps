#include "indexer/altitude_loader.hpp"

#include "coding/reader.hpp"
#include "coding/succinct_mapper.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include "defines.hpp"

#include "3party/succinct/mapper.hpp"

namespace
{
template<class TCont>
void Map(size_t dataSize, ReaderSource<FilesContainerR::TReader> & src,
         TCont & cont, unique_ptr<CopiedMemoryRegion> & region)
{
  vector<uint8_t> data(dataSize);
  src.Read(data.data(), data.size());
  region = make_unique<CopiedMemoryRegion>(move(data));
  coding::MapVisitor visitor(region->ImmutableData());
  cont.map(visitor);
}
} // namespace

namespace feature
{
AltitudeLoader::AltitudeLoader(MwmValue const & mwmValue)
{
  if (mwmValue.GetHeader().GetFormat() < version::Format::v8 )
    return;

  if (!mwmValue.m_cont.IsExist(ALTITUDES_FILE_TAG))
    return;

  try
  {
    m_reader = make_unique<FilesContainerR::TReader>(mwmValue.m_cont.GetReader(ALTITUDES_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    m_header.Deserialize(src);

    Map(m_header.GetAltitudeAvailabilitySize(), src, m_altitudeAvailability, m_altitudeAvailabilityRegion);
    Map(m_header.GetFeatureTableSize(), src, m_featureTable, m_featureTableRegion);
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR, ("Error while reading", ALTITUDES_FILE_TAG, "section.", e.Msg()));
  }
}

bool AltitudeLoader::IsAvailable() const
{
  return m_header.minAltitude != kInvalidAltitude;
}

TAltitudes const & AltitudeLoader::GetAltitudes(uint32_t featureId, size_t pointCount) const
{
  if (!IsAvailable())
  {
    // The version of mwm is less then version::Format::v8 or there's no altitude section in mwm.
    return m_dummy;
  }

  auto const it = m_cache.find(featureId);
  if (it != m_cache.end())
    return it->second;

  if (!m_altitudeAvailability[featureId])
  {
    LOG(LINFO, ("Feature featureId =", featureId, "does not contain any altitude information."));
    return m_cache.insert(make_pair(featureId, TAltitudes())).first->second;
  }

  uint64_t const r = m_altitudeAvailability.rank(featureId);
  CHECK_LESS(r, m_altitudeAvailability.size(), (featureId));
  uint64_t const offset = m_featureTable.select(r);
  CHECK_LESS_OR_EQUAL(offset, m_featureTable.size(), (featureId));

  uint64_t const altitudeInfoOffsetInSection = m_header.altitudeInfoOffset + offset;
  CHECK_LESS(altitudeInfoOffsetInSection, m_reader->Size(), ());

  try
  {
    Altitude a;
    ReaderSource<FilesContainerR::TReader> src(*m_reader);
    src.Skip(altitudeInfoOffsetInSection);
    a.Deserialize(m_header.minAltitude, pointCount, src);

    return m_cache.insert(make_pair(featureId, a.GetAltitudes())).first->second;
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Error while getting mwm data", e.Msg()));
    return m_cache.insert(make_pair(featureId, TAltitudes())).first->second;
  }
}
} // namespace feature
