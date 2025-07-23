#include "drape/overlay_handle.hpp"

#include "base/internal/message.hpp"
#include "base/macros.hpp"

#include "indexer/drawing_rule_def.hpp"

#include <algorithm>
#include <sstream>

namespace dp
{
struct OverlayHandle::OffsetNodeFinder
{
public:
  explicit OffsetNodeFinder(uint8_t bufferID) : m_bufferID(bufferID) {}

  bool operator()(OverlayHandle::TOffsetNode const & node) const { return node.first.GetID() == m_bufferID; }

private:
  uint8_t m_bufferID;
};

OverlayHandle::OverlayHandle(OverlayID const & id, dp::Anchor anchor, uint64_t priority, uint8_t minVisibleScale,
                             bool isBillboard)
  : m_id(id)
  , m_anchor(anchor)
  , m_priority(priority)
  , m_extendingSize(0.0)
  , m_pivotZ(0.0)
  , m_overlayRank(OverlayRank0)
  , m_minVisibleScale(minVisibleScale)
  , m_isBillboard(isBillboard)
  , m_isVisible(false)
  , m_caching(false)
  , m_extendedShapeDirty(true)
  , m_extendedRectDirty(true)
  , m_isReady(false)
  , m_isSpecialLayerOverlay(false)
  , m_displayFlag(false)
{}

void OverlayHandle::EnableCaching(bool enable)
{
  m_caching = enable;
  m_extendedShapeDirty = true;
  m_extendedRectDirty = true;
}

m2::PointD OverlayHandle::GetPivot(ScreenBase const & screen, bool perspective) const
{
  m2::RectD r = GetPixelRect(screen, false);
  m2::PointD size(0.5 * r.SizeX(), 0.5 * r.SizeY());
  m2::PointD result = r.Center();

  if (m_anchor & dp::Left)
    result.x -= size.x;
  else if (m_anchor & dp::Right)
    result.x += size.x;

  if (m_anchor & dp::Top)
    result.y -= size.y;
  else if (m_anchor & dp::Bottom)
    result.y += size.y;

  if (perspective)
    result = screen.PtoP3d(result, -m_pivotZ);

  return result;
}

bool OverlayHandle::IsIntersect(ScreenBase const & screen, ref_ptr<OverlayHandle> const h) const
{
  Rects const & ar1 = GetExtendedPixelShape(screen);
  Rects const & ar2 = h->GetExtendedPixelShape(screen);

  for (size_t i = 0; i < ar1.size(); ++i)
  {
    for (size_t j = 0; j < ar2.size(); ++j)
      if (ar1[i].IsIntersect(ar2[j]))
        return true;
  }
  return false;
}

void * OverlayHandle::IndexStorage(uint32_t size)
{
  m_indexes.Resize(size);
  return m_indexes.GetRaw();
}

void OverlayHandle::GetElementIndexes(ref_ptr<IndexBufferMutator> mutator) const
{
  ASSERT_EQUAL(m_isVisible, true, ());
  mutator->AppendIndexes(m_indexes.GetRawConst(), m_indexes.Size());
}

void OverlayHandle::GetAttributeMutation(ref_ptr<AttributeBufferMutator> mutator) const
{
  UNUSED_VALUE(mutator);
}

bool OverlayHandle::HasDynamicAttributes() const
{
  return !m_offsets.empty();
}

void OverlayHandle::AddDynamicAttribute(BindingInfo const & binding, uint32_t offset, uint32_t count)
{
  ASSERT(binding.IsDynamic(), ());
  VERIFY(m_offsets.emplace(binding, MutateRegion(offset, count)).second, ());
}

OverlayHandle::TOffsetNode const & OverlayHandle::GetOffsetNode(uint8_t bufferID) const
{
  auto const it = m_offsets.find(bufferID);
  ASSERT(it != m_offsets.end(), ());
  return *it;
}

m2::RectD OverlayHandle::GetExtendedPixelRect(ScreenBase const & screen) const
{
  if (m_caching && !m_extendedRectDirty)
    return m_extendedRectCache;

  m_extendedRectCache = GetPixelRect(screen, screen.isPerspective());
  m_extendedRectCache.Inflate(m_extendingSize, m_extendingSize);
  m_extendedRectDirty = false;
  return m_extendedRectCache;
}

OverlayHandle::Rects const & OverlayHandle::GetExtendedPixelShape(ScreenBase const & screen) const
{
  if (m_caching && !m_extendedShapeDirty)
    return m_extendedShapeCache;

  m_extendedShapeCache.clear();
  GetPixelShape(screen, screen.isPerspective(), m_extendedShapeCache);
  for (auto & rect : m_extendedShapeCache)
    rect.Inflate(m_extendingSize, m_extendingSize);
  m_extendedShapeDirty = false;
  return m_extendedShapeCache;
}

m2::RectD OverlayHandle::GetPerspectiveRect(m2::RectD const & pixelRect, ScreenBase const & screen) const
{
  m2::PointD const tmpPoint = screen.PtoP3d(pixelRect.LeftTop());
  m2::RectD perspectiveRect(tmpPoint, tmpPoint);
  perspectiveRect.Add(screen.PtoP3d(pixelRect.LeftBottom()));
  perspectiveRect.Add(screen.PtoP3d(pixelRect.RightBottom()));
  perspectiveRect.Add(screen.PtoP3d(pixelRect.RightTop()));

  return perspectiveRect;
}

m2::RectD OverlayHandle::GetPixelRectPerspective(ScreenBase const & screen) const
{
  if (m_isBillboard)
  {
    m2::PointD const pxPivot = GetPivot(screen, false);
    m2::PointD const pxPivotPerspective = screen.PtoP3d(pxPivot, -m_pivotZ);

    m2::RectD pxRectPerspective = GetPixelRect(screen, false);
    pxRectPerspective.Offset(-pxPivot);
    pxRectPerspective.Offset(pxPivotPerspective);

    return pxRectPerspective;
  }

  return GetPerspectiveRect(GetPixelRect(screen, false), screen);
}

SquareHandle::SquareHandle(OverlayID const & id, dp::Anchor anchor, m2::PointD const & gbPivot,
                           m2::PointD const & pxSize, m2::PointD const & pxOffset, uint64_t priority, bool isBound,
                           int minVisibleScale, bool isBillboard)
  : TBase(id, anchor, priority, minVisibleScale, isBillboard)
  , m_pxHalfSize(pxSize.x / 2.0, pxSize.y / 2.0)
  , m_gbPivot(gbPivot)
  , m_pxOffset(pxOffset)
  , m_isBound(isBound)
{}

m2::RectD SquareHandle::GetPixelRect(ScreenBase const & screen, bool perspective) const
{
  if (perspective)
    return GetPixelRectPerspective(screen);

  m2::PointD const pxPivot = screen.GtoP(m_gbPivot) + m_pxOffset;
  m2::RectD result(pxPivot - m_pxHalfSize, pxPivot + m_pxHalfSize);
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

void SquareHandle::GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const
{
  rects.emplace_back(GetPixelRect(screen, perspective));
}

bool SquareHandle::IsBound() const
{
  return m_isBound;
}

#ifdef DEBUG_OVERLAYS_OUTPUT
std::string SquareHandle::GetOverlayDebugInfo()
{
  std::ostringstream out;
  out << "POI Priority(" << std::hex << std::setw(16) << std::setfill('0') << GetPriority() << ") " << std::dec
      << DebugPrint(GetOverlayID());
  return out.str();
}
#endif

/// @param[in] rank         Rank of the feature (bigger is better)
/// @param[in] depth        Manual priority from styles (bigger is better)
uint64_t CalculateOverlayPriority(uint8_t rank, float depth)
{
  // Negative range is used for optional captions which are below all other overlays.
  ASSERT(-drule::kOverlaysMaxPriority <= depth && depth < drule::kOverlaysMaxPriority, (depth));
  depth += drule::kOverlaysMaxPriority;

  // Pack into uint64_t priority value (bigger is better).
  // [1 byte - 0xFF][4 bytes - priority][1 byte - rank][2 bytes - 0xFFFF].
  return (static_cast<uint64_t>(depth) << 24) | (static_cast<uint64_t>(rank) << 16) | dp::kPriorityMaskZoomLevel;
}

uint64_t CalculateSpecialModeUserMarkPriority(uint16_t specialPriority)
{
  // [6 bytes - 0xFFFFFFFFFFFF][2 bytes - special priority]
  static uint64_t constexpr kMask = ~static_cast<uint64_t>(0xFFFF);
  uint64_t priority = dp::kPriorityMaskAll;
  priority &= kMask;
  priority |= specialPriority;
  return priority;
}

uint64_t CalculateUserMarkPriority(int minZoomLevel, uint16_t specialPriority)
{
  // [1 byte - zoom][5 bytes - 0xFFFFFFFFFF][2 bytes - special priority]
  uint8_t const minZoom = 0xFF - static_cast<uint8_t>(std::max(minZoomLevel, 0));
  uint64_t priority = ~dp::kPriorityMaskZoomLevel;
  return priority | (static_cast<uint64_t>(minZoom) << 56) | static_cast<uint64_t>(specialPriority);
}
}  // namespace dp
