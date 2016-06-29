#include "indexer/altitude_loader.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

namespace feature
{
AltitudeLoader::AltitudeLoader(MwmValue const * mwmValue)
{
  if (!mwmValue || mwmValue->GetHeader().GetFormat() < version::Format::v8 )
    return;

  try
  {
    m_idx = make_unique<DDVector<TAltitudeIndexEntry, FilesContainerR::TReader>>
        (mwmValue->m_cont.GetReader(ALTITUDE_FILE_TAG));
  }
  catch (Reader::OpenException const &)
  {
    LOG(LINFO, ("MWM does not contain", ALTITUDE_FILE_TAG, "section."));
  }
}

Altitudes AltitudeLoader::GetAltitudes(uint32_t featureId) const
{
  if (!m_idx || m_idx->size() == 0)
    return Altitudes();

  auto it = lower_bound(m_idx->begin(), m_idx->end(),
                        TAltitudeIndexEntry{static_cast<uint32_t>(featureId), 0, 0},
                        my::LessBy(&TAltitudeIndexEntry::featureId));

  if (it == m_idx->end())
    return Altitudes();

  if (featureId != it->featureId)
  {
    ASSERT(false, ());
    return Altitudes();
  }

  if (it->beginAlt == kInvalidAltitude || it->endAlt == kInvalidAltitude)
    return Altitudes();

  return Altitudes(it->beginAlt, it->endAlt);
}
} // namespace feature
