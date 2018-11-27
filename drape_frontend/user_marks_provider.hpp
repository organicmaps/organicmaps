#pragma once

#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "kml/type_utils.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/polyline2d.hpp"

#include <vector>

namespace df
{
struct IDCollections
{
  kml::MarkIdCollection m_markIds;
  kml::TrackIdCollection m_lineIds;

  bool IsEmpty()
  {
    return m_markIds.empty() && m_lineIds.empty();
  }

  void Clear()
  {
    m_markIds.clear();
    m_lineIds.clear();
  }
};

class UserPointMark
{
public:
  struct ColoredSymbolZoomInfo
  {
    std::map<int, df::ColoredSymbolViewParams> m_zoomInfo;
    bool m_needOverlay = true;
    bool m_addTextSize = false;
  };
  using SymbolNameZoomInfo = std::map<int, std::string>;
  using TitlesInfo = std::vector<dp::TitleDecl>;
  using SymbolSizes = std::vector<m2::PointF>;
  using SymbolOffsets = std::vector<m2::PointF>;

  explicit UserPointMark(kml::MarkId id);
  virtual ~UserPointMark() = default;

  virtual bool IsDirty() const = 0;
  virtual void ResetChanges() const = 0;

  kml::MarkId GetId() const { return m_id; }
  virtual kml::MarkGroupId GetGroupId() const = 0;

  virtual m2::PointD const & GetPivot() const = 0;
  virtual m2::PointD GetPixelOffset() const = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual bool GetDepthTestEnabled() const = 0;
  virtual float GetDepth() const = 0;
  virtual DepthLayer GetDepthLayer() const = 0;
  virtual bool IsVisible() const = 0;
  virtual drape_ptr<TitlesInfo> GetTitleDecl() const = 0;
  virtual drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const = 0;
  virtual drape_ptr<SymbolNameZoomInfo> GetBadgeNames() const = 0;
  virtual drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const = 0;
  virtual drape_ptr<SymbolSizes> GetSymbolSizes() const = 0;
  virtual drape_ptr<SymbolOffsets> GetSymbolOffsets() const = 0;
  virtual uint16_t GetPriority() const = 0;
  virtual df::SpecialDisplacement GetDisplacement() const = 0;
  virtual uint32_t GetIndex() const = 0;
  virtual bool HasSymbolShapes() const = 0;
  virtual bool HasTitlePriority() const = 0;
  virtual int GetMinZoom() const = 0;
  virtual int GetMinTitleZoom() const = 0;
  virtual FeatureID GetFeatureID() const = 0;
  virtual bool HasCreationAnimation() const = 0;
  virtual df::ColorConstant GetColorConstant() const = 0;
  virtual bool IsMarkAboveText() const = 0;

private:
  kml::MarkId m_id;
};

class UserLineMark
{
public:
  explicit UserLineMark(kml::TrackId id);
  virtual ~UserLineMark() = default;

  virtual bool IsDirty() const = 0;
  virtual void ResetChanges() const = 0;

  virtual kml::TrackId GetId() const { return m_id; }

  virtual int GetMinZoom() const = 0;
  virtual DepthLayer GetDepthLayer() const = 0;
  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetDepth(size_t layerIndex) const = 0;
  virtual std::vector<m2::PointD> const & GetPoints() const = 0;

private:
  kml::TrackId m_id;
};

class UserMarksProvider
{
public:
  virtual ~UserMarksProvider() = default;
  virtual kml::GroupIdSet const & GetDirtyGroupIds() const = 0;
  virtual kml::GroupIdSet const & GetRemovedGroupIds() const = 0;
  virtual kml::GroupIdSet GetAllGroupIds() const = 0;
  virtual bool IsGroupVisible(kml::MarkGroupId groupId) const = 0;
  virtual bool IsGroupVisibilityChanged(kml::MarkGroupId groupId) const = 0;
  virtual kml::MarkIdSet const & GetGroupPointIds(kml::MarkGroupId groupId) const = 0;
  virtual kml::TrackIdSet const & GetGroupLineIds(kml::MarkGroupId groupId) const = 0;
  virtual kml::MarkIdSet const & GetCreatedMarkIds() const = 0;
  virtual kml::MarkIdSet const & GetRemovedMarkIds() const = 0;
  virtual kml::MarkIdSet const & GetUpdatedMarkIds() const = 0;
  virtual kml::TrackIdSet const & GetRemovedLineIds() const = 0;
  /// Never store UserPointMark reference.
  virtual UserPointMark const * GetUserPointMark(kml::MarkId markId) const = 0;
  /// Never store UserLineMark reference.
  virtual UserLineMark const * GetUserLineMark(kml::TrackId lineId) const = 0;
};
}  // namespace df
