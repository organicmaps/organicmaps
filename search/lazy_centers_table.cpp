#include "search/lazy_centers_table.hpp"

#include "indexer/mwm_set.hpp"

#include "defines.hpp"

namespace search
{
LazyCentersTable::LazyCentersTable(MwmValue & value)
  : m_value(value)
  , m_state(STATE_NOT_LOADED)
  , m_reader(unique_ptr<ModelReader>())
{
}

void LazyCentersTable::EnsureTableLoaded()
{
  if (m_state != STATE_NOT_LOADED)
    return;

  if (!m_value.m_cont.IsExist(CENTERS_FILE_TAG))
  {
    m_state = STATE_FAILED;
    return;
  }

  m_reader = m_value.m_cont.GetReader(CENTERS_FILE_TAG);
  if (!m_reader.GetPtr())
  {
    m_state = STATE_FAILED;
    return;
  }

  m_table =
      CentersTable::Load(*m_reader.GetPtr(), m_value.GetHeader().GetDefGeometryCodingParams());
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
