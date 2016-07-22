#include "indexer/altitude_loader.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/thread.hpp"

#include "defines.hpp"

#include "3party/succinct/mapper.hpp"

namespace
{
void ReadBuffer(ReaderSource<FilesContainerR::TReader> & rs, vector<char> & buf)
{
  uint32_t bufSz = 0;
  rs.Read(&bufSz, sizeof(bufSz));
  if (bufSz > rs.Size() + rs.Pos())
  {
    ASSERT(false, ());
    return;
  }
  buf.clear();
  buf.resize(bufSz);
  rs.Read(buf.data(), bufSz);
}
} // namespace

namespace feature
{
AltitudeLoader::AltitudeLoader(MwmValue const * mwmValue)
{
  if (!mwmValue || mwmValue->GetHeader().GetFormat() < version::Format::v8 )
    return;

  if (!mwmValue->m_cont.IsExist(ALTITUDES_FILE_TAG))
    return;

  try
  {
    m_reader = make_unique<FilesContainerR::TReader>(mwmValue->m_cont.GetReader(ALTITUDES_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> rs(*m_reader);
    m_header.Deserialize(rs);

    // Reading rs_bit_vector with altitude availability information.
    ReadBuffer(rs, m_altitudeAvailabilitBuf);
    m_altitudeAvailability = make_unique<succinct::rs_bit_vector>();
    succinct::mapper::map(*m_altitudeAvailability, m_altitudeAvailabilitBuf.data());

    // Reading table with altitude ofsets for features.
    ReadBuffer(rs, m_featureTableBuf);
    m_featureTable = make_unique<succinct::elias_fano>();
    succinct::mapper::map(*m_featureTable, m_featureTableBuf.data());
  }
  catch (Reader::OpenException const & e)
  {
    m_header.Reset();
    LOG(LERROR, ("Error while reading", ALTITUDES_FILE_TAG, "section.", e.Msg()));
  }
}

bool AltitudeLoader::IsAvailable() const
{
  return m_header.minAltitude != kInvalidAltitude && m_header.altitudeInfoOffset != 0;
}

TAltitudes AltitudeLoader::GetAltitude(uint32_t featureId, size_t pointCount) const
{
  if (m_header.altitudeInfoOffset == 0)
  {
    // The version of mwm is less then version::Format::v8 or there's no altitude section in mwm.
    return TAltitudes();
  }

  if (!(*m_altitudeAvailability)[featureId])
  {
    LOG(LINFO, ("Feature featureId =", featureId, "does not contain any altitude information."));
    return TAltitudes();
  }

  uint64_t const r = m_altitudeAvailability->rank(featureId);
  CHECK_LESS(r, m_altitudeAvailability->size(), (featureId));
  uint64_t const offset = m_featureTable->select(r);
  CHECK_LESS_OR_EQUAL(offset, m_featureTable->size(), (featureId));

  uint64_t const altitudeInfoOffsetInSection = m_header.altitudeInfoOffset + offset;
  CHECK_LESS(altitudeInfoOffsetInSection, m_reader->Size(), ());

  try
  {
    Altitude a;
    ReaderSource<FilesContainerR::TReader> rs(*m_reader);
    rs.Skip(altitudeInfoOffsetInSection);
    a.Deserialize(m_header.minAltitude, pointCount, rs);

    // @TODO(bykoianko) Considers using move semantic for returned value here.
    return a.GetAltitudes();
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("Error while getting mwm data", e.Msg()));
    return TAltitudes();
  }
}
} // namespace feature
