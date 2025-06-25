#pragma once

#include "drape/attribute_buffer_mutator.hpp"
#include "drape/binding_info.hpp"
#include "drape/drape_diagnostics.hpp"
#include "drape/drape_global.hpp"
#include "drape/index_buffer_mutator.hpp"
#include "drape/index_storage.hpp"

#include "kml/type_utils.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/string_utils.hpp"

#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace dp
{
enum OverlayRank : uint8_t
{
  OverlayRank0 = 0,
  OverlayRank1,
  OverlayRank2,
  OverlayRank3,
  OverlayRanksCount
};

uint64_t constexpr kPriorityMaskZoomLevel = 0xFF0000000000FFFF;
uint64_t constexpr kPriorityMaskAll = std::numeric_limits<uint64_t>::max();

struct OverlayID
{
  FeatureID m_featureId;
  kml::MarkId m_markId = kml::kInvalidMarkId;
  m2::PointI m_tileCoords{-1, -1};
  uint32_t m_index = 0;

  OverlayID() = default;

  explicit OverlayID(FeatureID const & featureId) : m_featureId(featureId) {}

  OverlayID(FeatureID const & featureId, kml::MarkId markId, m2::PointI const & tileCoords, uint32_t index)
    : m_featureId(featureId)
    , m_markId(markId)
    , m_tileCoords(tileCoords)
    , m_index(index)
  {}

  bool IsValid() const { return m_featureId.IsValid() || m_markId != kml::kInvalidMarkId; }

  static OverlayID GetLowerKey(FeatureID const & featureID) { return {featureID, 0, {-1, -1}, 0}; }

  auto AsTupleOfRefs() const { return std::tie(m_featureId, m_markId, m_tileCoords, m_index); }

  bool operator==(OverlayID const & overlayId) const { return AsTupleOfRefs() == overlayId.AsTupleOfRefs(); }

  bool operator!=(OverlayID const & overlayId) const { return !operator==(overlayId); }

  bool operator<(OverlayID const & overlayId) const { return AsTupleOfRefs() < overlayId.AsTupleOfRefs(); }

  bool operator>(OverlayID const & overlayId) const { return overlayId < *this; }

  friend std::string DebugPrint(OverlayID const & overlayId)
  {
    return strings::to_string(overlayId.m_featureId.m_index) + "-" + strings::to_string(overlayId.m_markId) + "-" +
           strings::to_string(overlayId.m_index);
  }
};

class OverlayHandle
{
public:
  using Rects = std::vector<m2::RectF>;

  OverlayHandle(OverlayID const & id, dp::Anchor anchor, uint64_t priority, uint8_t minVisibleScale, bool isBillboard);

  virtual ~OverlayHandle() = default;

  bool IsVisible() const { return m_isVisible; }
  void SetIsVisible(bool isVisible) { m_isVisible = isVisible; }

  uint8_t GetMinVisibleScale() const { return m_minVisibleScale; }
  bool IsBillboard() const { return m_isBillboard; }

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

  OverlayID const & GetOverlayID() const { return m_id; }
  uint64_t const & GetPriority() const { return m_priority; }

  virtual bool IsBound() const { return false; }
  virtual bool HasLinearFeatureShape() const { return false; }

  virtual bool Enable3dExtention() const { return true; }

  using RankT = uint8_t;  // Same as OverlayRank
  RankT GetOverlayRank() const { return m_overlayRank; }
  void SetOverlayRank(RankT overlayRank) { m_overlayRank = overlayRank; }

  void EnableCaching(bool enable);
  bool IsCachingEnabled() const { return m_caching; }

  void SetReady(bool isReady) { m_isReady = isReady; }
  bool IsReady() const { return m_isReady; }

  void SetDisplayFlag(bool display) { m_displayFlag = display; }
  /// @todo displayFlag logic is effectively turned off now,
  /// remove all the associated code from drape later if its not needed anymore.
  // The displayFlag displacement logic supposedly should stabilize displacement
  // chains and prevent "POI blinking" cases by remembering which POIs were visible
  // on a previous iteration and giving them priority over "new" POIs.
  // In reality it seems to be of very little (if any) benefit but causes major issues:
  //  - minor POIs may displace major ones just because they were displayed previously;
  //  - testing priority changes and debugging displacement becomes very hard as map browsing history
  //    prevails over priorities, so displacement results are unpredictable.
  bool GetDisplayFlag() const { return true; /* m_displayFlag; */ }

  void SetSpecialLayerOverlay(bool isSpecialLayerOverlay) { m_isSpecialLayerOverlay = isSpecialLayerOverlay; }
  bool IsSpecialLayerOverlay() const { return m_isSpecialLayerOverlay; }

#ifdef DEBUG_OVERLAYS_OUTPUT
  virtual std::string GetOverlayDebugInfo() { return ""; }
#endif

protected:
  OverlayID const m_id;
  dp::Anchor const m_anchor;
  uint64_t const m_priority;

  double m_extendingSize;
  double m_pivotZ;
  RankT m_overlayRank;

  using TOffsetNode = std::pair<BindingInfo, MutateRegion>;
  TOffsetNode const & GetOffsetNode(uint8_t bufferID) const;

  m2::RectD GetPerspectiveRect(m2::RectD const & pixelRect, ScreenBase const & screen) const;
  m2::RectD GetPixelRectPerspective(ScreenBase const & screen) const;

private:
  uint8_t m_minVisibleScale;

  dp::IndexStorage m_indexes;
  struct LessOffsetNode
  {
    typedef bool is_transparent;

    bool operator()(TOffsetNode const & node1, TOffsetNode const & node2) const
    {
      return node1.first.GetID() < node2.first.GetID();
    }
    bool operator()(uint8_t node1, TOffsetNode const & node2) const { return node1 < node2.first.GetID(); }
    bool operator()(TOffsetNode const & node1, uint8_t node2) const { return node1.first.GetID() < node2; }
  };

  struct OffsetNodeFinder;

  std::set<TOffsetNode, LessOffsetNode> m_offsets;

  mutable Rects m_extendedShapeCache;
  mutable m2::RectD m_extendedRectCache;

  bool const m_isBillboard : 1;
  bool m_isVisible : 1;

  bool m_caching : 1;
  mutable bool m_extendedShapeDirty : 1;
  mutable bool m_extendedRectDirty : 1;

  bool m_isReady : 1;
  bool m_isSpecialLayerOverlay : 1;
  bool m_displayFlag : 1;
};

class SquareHandle : public OverlayHandle
{
  using TBase = OverlayHandle;

public:
  SquareHandle(OverlayID const & id, dp::Anchor anchor, m2::PointD const & gbPivot, m2::PointD const & pxSize,
               m2::PointD const & pxOffset, uint64_t priority, bool isBound, int minVisibleScale, bool isBillboard);

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
};

uint64_t CalculateOverlayPriority(uint8_t rank, float depth);
uint64_t CalculateSpecialModeUserMarkPriority(uint16_t specialPriority);
uint64_t CalculateUserMarkPriority(int minZoomLevel, uint16_t specialPriority);
}  // namespace dp
