#pragma once

#include "drape/drape_diagnostics.hpp"
#include "drape/drape_global.hpp"
#include "drape/binding_info.hpp"
#include "drape/index_buffer_mutator.hpp"
#include "drape/index_storage.hpp"
#include "drape/attribute_buffer_mutator.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/screenbase.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace dp
{
enum OverlayRank
{
  OverlayRank0 = 0,
  OverlayRank1,
  OverlayRank2,
  OverlayRank3,
  OverlayRanksCount
};

uint64_t constexpr kPriorityMaskZoomLevel = 0xFF0000000000FFFF;
uint64_t constexpr kPriorityMaskManual    = 0x00FFFFFFFF00FFFF;
uint64_t constexpr kPriorityMaskRank      = 0x0000000000FFFFFF;
uint64_t constexpr kPriorityMaskAll = kPriorityMaskZoomLevel |
                                      kPriorityMaskManual |
                                      kPriorityMaskRank;
struct OverlayID
{
  FeatureID m_featureId;
  m2::PointI m_tileCoords;
  uint32_t m_index;

  OverlayID(FeatureID const & featureId)
    : m_featureId(featureId), m_tileCoords(-1, -1), m_index(0)
  {}
  OverlayID(FeatureID const & featureId, m2::PointI const & tileCoords, uint32_t index)
    : m_featureId(featureId), m_tileCoords(tileCoords), m_index(index)
  {}

  bool operator==(OverlayID const & overlayId) const
  {
    return m_featureId == overlayId.m_featureId && m_tileCoords == overlayId.m_tileCoords &&
           m_index == overlayId.m_index;
  }

  bool operator!=(OverlayID const & overlayId) const
  {
    return !operator ==(overlayId);
  }

  bool operator<(OverlayID const & overlayId) const
  {
    if (m_featureId == overlayId.m_featureId)
    {
      if (m_tileCoords == overlayId.m_tileCoords)
        return m_index < overlayId.m_index;

      return m_tileCoords < overlayId.m_tileCoords;
    }
    return m_featureId < overlayId.m_featureId;
  }

  bool operator>(OverlayID const & overlayId) const
  {
    if (m_featureId == overlayId.m_featureId)
    {
      if (m_tileCoords == overlayId.m_tileCoords)
        return m_index > overlayId.m_index;

      return !(m_tileCoords < overlayId.m_tileCoords);
    }
    return !(m_featureId < overlayId.m_featureId);
  }
};

class OverlayHandle
{
public:
  using Rects = std::vector<m2::RectF>;

  OverlayHandle(OverlayID const & id, dp::Anchor anchor,
                uint64_t priority, int minVisibleScale, bool isBillboard);

  virtual ~OverlayHandle() {}

  bool IsVisible() const;
  void SetIsVisible(bool isVisible);

  int GetMinVisibleScale() const;
  bool IsBillboard() const;

  virtual m2::PointD GetPivot(ScreenBase const & screen, bool perspective) const;

  virtual void BeforeUpdate() {}
  virtual bool Update(ScreenBase const & /*screen*/) { return true; }

  virtual m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const = 0;
  virtual void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const = 0;

  double GetPivotZ() const { return m_pivotZ; }
  void SetPivotZ(double pivotZ) { m_pivotZ = pivotZ; }

  double GetExtendingSize() const { return m_extendingSize; }
  void SetExtendingSize(double extendingSize) { m_extendingSize = extendingSize; }
  m2::RectD GetExtendedPixelRect(ScreenBase const & screen) const;
  Rects const & GetExtendedPixelShape(ScreenBase const & screen) const;

  bool IsIntersect(ScreenBase const & screen, ref_ptr<OverlayHandle> const h) const;

  virtual bool IndexesRequired() const { return true; }
  void * IndexStorage(uint32_t size);
  void GetElementIndexes(ref_ptr<IndexBufferMutator> mutator) const;
  virtual void GetAttributeMutation(ref_ptr<AttributeBufferMutator> mutator) const;

  bool HasDynamicAttributes() const;
  void AddDynamicAttribute(BindingInfo const & binding, uint32_t offset, uint32_t count);

  OverlayID const & GetOverlayID() const;
  uint64_t const & GetPriority() const;

  virtual uint64_t GetPriorityMask() const { return kPriorityMaskAll; }

  virtual bool IsBound() const { return false; }
  virtual bool HasLinearFeatureShape() const { return false; }

  virtual bool Enable3dExtention() const { return true; }

  int GetOverlayRank() const { return m_overlayRank; }
  void SetOverlayRank(int overlayRank) { m_overlayRank = overlayRank; }

  void SetCachingEnable(bool enable);

  void SetReady(bool isReady) { m_isReady = isReady; }
  bool IsReady() const { return m_isReady; }

  void SetDisplayFlag(bool display) { m_displayFlag = display; }
  bool GetDisplayFlag() const { return m_displayFlag; }

  void SetSpecialLayerOverlay(bool isSpecialLayerOverlay) { m_isSpecialLayerOverlay = isSpecialLayerOverlay; }
  bool IsSpecialLayerOverlay() const { return m_isSpecialLayerOverlay; }

#ifdef DEBUG_OVERLAYS_OUTPUT
  virtual std::string GetOverlayDebugInfo() { return ""; }
#endif

protected:
  OverlayID const m_id;
  dp::Anchor const m_anchor;
  uint64_t const m_priority;

  int m_overlayRank;
  double m_extendingSize;
  double m_pivotZ;

  using TOffsetNode = std::pair<BindingInfo, MutateRegion>;
  TOffsetNode const & GetOffsetNode(uint8_t bufferID) const;

  m2::RectD GetPerspectiveRect(m2::RectD const & pixelRect, ScreenBase const & screen) const;
  m2::RectD GetPixelRectPerspective(ScreenBase const & screen) const;

private:
  int m_minVisibleScale;
  bool const m_isBillboard;
  bool m_isVisible;

  dp::IndexStorage m_indexes;
  struct LessOffsetNode
  {
    bool operator()(TOffsetNode const & node1, TOffsetNode const & node2) const
    {
      return node1.first.GetID() < node2.first.GetID();
    }
  };

  struct OffsetNodeFinder;

  std::set<TOffsetNode, LessOffsetNode> m_offsets;

  bool m_enableCaching;
  mutable Rects m_extendedShapeCache;
  mutable bool m_extendedShapeDirty;
  mutable m2::RectD m_extendedRectCache;
  mutable bool m_extendedRectDirty;

  bool m_isReady = false;
  bool m_isSpecialLayerOverlay = false;
  bool m_displayFlag = false;
};

class SquareHandle : public OverlayHandle
{
  using TBase = OverlayHandle;

public:
  SquareHandle(OverlayID const & id, dp::Anchor anchor, m2::PointD const & gbPivot,
               m2::PointD const & pxSize, m2::PointD const & pxOffset,
               uint64_t priority, bool isBound, std::string const & debugStr,
               int minVisibleScale, bool isBillboard);

  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;
  bool IsBound() const override;

#ifdef DEBUG_OVERLAYS_OUTPUT
  virtual std::string GetOverlayDebugInfo() override;
#endif

protected:
  m2::PointD m_pxHalfSize;

private:
  m2::PointD m_gbPivot;
  m2::PointD m_pxOffset;
  bool m_isBound;

#ifdef DEBUG_OVERLAYS_OUTPUT
  std::string m_debugStr;
#endif
};

uint64_t CalculateOverlayPriority(int minZoomLevel, uint8_t rank, float depth);
uint64_t CalculateSpecialModePriority(uint16_t specialPriority);
uint64_t CalculateSpecialModeUserMarkPriority(uint16_t specialPriority);
uint64_t CalculateUserMarkPriority(int minZoomLevel, uint16_t specialPriority);
}  // namespace dp
