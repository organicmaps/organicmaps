#include "overlay_handle.hpp"

OverlayHandle::OverlayHandle(const FeatureID & id, OverlayHandle::Anchor anchor, m2::PointD const & gbPivot,
                             m2::PointD const & pxSize, double priority)
  : m_id(id)
  , m_anchor(anchor)
  , m_gbPivot(gbPivot)
  , m_pxHalfSize(pxSize.x / 2.0, pxSize.y / 2.0)
  , m_priority(priority)
  , m_isVisible(false)
{
}

bool OverlayHandle::IsVisible() const
{
  return m_isVisible;
}

void OverlayHandle::SetIsVisible(bool isVisible)
{
  m_isVisible = isVisible;
}

m2::RectD OverlayHandle::GetPixelRect(ScreenBase const & screen) const
{
  m2::PointD pxPivot = screen.GtoP(m_gbPivot);
  m2::RectD  result(pxPivot - m_pxHalfSize, pxPivot + m_pxHalfSize);
  m2::PointD offset(0.0, 0.0);

  if (m_anchor & Left)
    offset.x = m_pxHalfSize.x;
  else if (m_anchor & Right)
    offset.x = -m_pxHalfSize.x;

  if (m_anchor & Top)
    offset.y = m_pxHalfSize.y;
  else if (m_anchor & Bottom)
    offset.y = -m_pxHalfSize.y;

  result.Offset(offset);
  return result;
}

uint16_t * OverlayHandle::IndexStorage(uint16_t size)
{
  m_indexes.resize(size);
  return &m_indexes[0];
}

size_t OverlayHandle::GetIndexCount() const
{
  return m_indexes.size();
}

void OverlayHandle::GetElementIndexes(RefPointer<IndexBufferMutator> mutator) const
{
  ASSERT_EQUAL(m_isVisible, true, ());
  mutator->AppendIndexes(&m_indexes[0], m_indexes.size());
}

FeatureID const & OverlayHandle::GetFeatureID() const
{
  return m_id;
}

double const & OverlayHandle::GetPriority() const
{
  return m_priority;
}
