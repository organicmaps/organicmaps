#include "overlay_handle.hpp"

#include "../base/macros.hpp"

struct OverlayHandle::OffsetNodeFinder
{
public:
  OffsetNodeFinder(uint8_t bufferID) : m_bufferID(bufferID) {}

  bool operator()(OverlayHandle::TOffsetNode const & node) const
  {
    return node.first.GetID() == m_bufferID;
  }

private:
  uint8_t m_bufferID;
};

OverlayHandle::OverlayHandle(FeatureID const & id,
                             dp::Anchor anchor,
                             double priority)
  : m_id(id)
  , m_anchor(anchor)
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

void OverlayHandle::GetAttributeMutation(RefPointer<AttributeBufferMutator> mutator) const
{
  UNUSED_VALUE(mutator);
}

bool OverlayHandle::HasDynamicAttributes() const
{
  return !m_offsets.empty();
}

void OverlayHandle::AddDynamicAttribute(BindingInfo const & binding, uint16_t offset, uint16_t count)
{
  ASSERT(binding.IsDynamic(), ());
  ASSERT(find_if(m_offsets.begin(), m_offsets.end(), OffsetNodeFinder(binding.GetID())) == m_offsets.end(), ());
  m_offsets.insert(make_pair(binding, MutateRegion(offset, count)));
}

FeatureID const & OverlayHandle::GetFeatureID() const
{
  return m_id;
}

double const & OverlayHandle::GetPriority() const
{
  return m_priority;
}

OverlayHandle::TOffsetNode const & OverlayHandle::GetOffsetNode(uint8_t bufferID) const
{
  set<TOffsetNode>::const_iterator it = find_if(m_offsets.begin(), m_offsets.end(), OffsetNodeFinder(bufferID));
  ASSERT(it != m_offsets.end(), ());
  return *it;
}

////////////////////////////////////////////////////////////////////////////////

SquareHandle::SquareHandle(FeatureID const & id, dp::Anchor anchor,
                           m2::PointD const & gbPivot, m2::PointD const & pxSize,
                           double priority)
  : base_t(id, anchor, priority)
  , m_gbPivot(gbPivot)
  , m_pxHalfSize(pxSize.x / 2.0, pxSize.y / 2.0)
{
}

m2::RectD SquareHandle::GetPixelRect(ScreenBase const & screen) const
{
  m2::PointD const pxPivot = screen.GtoP(m_gbPivot);
  m2::RectD  result(pxPivot - m_pxHalfSize, pxPivot + m_pxHalfSize);
  m2::PointD offset(0.0, 0.0);

  if (m_anchor & dp::Left)
    offset.x = m_pxHalfSize.x;
  else if (m_anchor & dp::Right)
    offset.x = -m_pxHalfSize.x;

  if (m_anchor & dp::Top)
    offset.y = m_pxHalfSize.y;
  else if (m_anchor & dp::Bottom)
    offset.y = -m_pxHalfSize.y;

  result.Offset(offset);
  return result;
}
