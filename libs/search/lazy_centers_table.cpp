#include "search/lazy_centers_table.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/mwm_traits.hpp"

#include "defines.hpp"

namespace search
{
LazyCentersTable::LazyCentersTable(MwmValue const & value)
  : m_value(value)
  , m_state(STATE_NOT_LOADED)
  , m_reader(std::unique_ptr<ModelReader>())
{}

void LazyCentersTable::EnsureTableLoaded()
{
  if (m_state != STATE_NOT_LOADED)
    return;

  try
  {
    m_reader = m_value.m_cont.GetReader(CENTERS_FILE_TAG);
  }
  catch (RootException const & ex)
  {
    LOG(LERROR, ("Unable to load", CENTERS_FILE_TAG, ex.Msg()));
    m_state = STATE_FAILED;
    return;
  }

  auto const format = version::MwmTraits(m_value.GetMwmVersion()).GetCentersTableFormat();
  CHECK_EQUAL(format, version::MwmTraits::CentersTableFormat::EliasFanoMapWithHeader, ());
  m_table = CentersTable::LoadV1(*m_reader.GetPtr());

  if (m_table)
    m_state = STATE_LOADED;
  else
    m_state = STATE_FAILED;
}

bool LazyCentersTable::Get(uint32_t id, m2::PointD & center)
{
  EnsureTableLoaded();
  if (m_state != STATE_LOADED)
    return false;
  return m_table->Get(id, center);
}
}  // namespace search
