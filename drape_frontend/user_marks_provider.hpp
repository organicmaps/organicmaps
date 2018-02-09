#pragma once

#include "drape_frontend/render_state.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/user_marks_global.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/polyline2d.hpp"

#include <vector>

namespace df
{
struct IDCollections
{
  MarkIDCollection m_marksID;
  LineIDCollection m_linesID;

  bool IsEmpty()
  {
    return m_marksID.empty() && m_linesID.empty();
  }

  void Clear()
  {
    m_marksID.clear();
    m_linesID.clear();
  }
};

class UserPointMark
{
public:
  using ColoredSymbolZoomInfo = std::map<int, df::ColoredSymbolViewParams>;
  using SymbolNameZoomInfo = std::map<int, std::string>;
  using TitlesInfo = std::vector<dp::TitleDecl>;
  using SymbolSizes = std::vector<m2::PointF>;
  using SymbolOffsets = std::vector<m2::PointF>;

  UserPointMark(df::MarkID id);
  virtual ~UserPointMark() {}

  virtual bool IsDirty() const = 0;
  virtual void ResetChanges() const = 0;

  MarkID GetId() const { return m_id; }
  virtual df::MarkGroupID GetGroupId() const = 0;

  virtual m2::PointD const & GetPivot() const = 0;
  virtual m2::PointD GetPixelOffset() const = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual float GetDepth() const = 0;
  virtual RenderState::DepthLayer GetDepthLayer() const = 0;
  virtual bool IsVisible() const = 0;
  virtual drape_ptr<TitlesInfo> GetTitleDecl() const = 0;
  virtual drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const = 0;
  virtual drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const = 0;
  virtual drape_ptr<SymbolSizes> GetSymbolSizes() const = 0;
  virtual drape_ptr<SymbolOffsets> GetSymbolOffsets() const = 0;
  virtual uint16_t GetPriority() const = 0;
  virtual uint32_t GetIndex() const = 0;
  virtual bool HasSymbolPriority() const = 0;
  virtual bool HasTitlePriority() const = 0;
  virtual int GetMinZoom() const = 0;
  virtual int GetMinTitleZoom() const = 0;
  virtual FeatureID GetFeatureID() const = 0;
  virtual bool HasCreationAnimation() const = 0;

private:
  MarkID m_id;
};

class UserLineMark
{
public:
  UserLineMark(df::LineID id);
  virtual ~UserLineMark() {}

  virtual bool IsDirty() const = 0;
  virtual void ResetChanges() const = 0;

  virtual LineID GetId() const { return m_id; }

  virtual int GetMinZoom() const = 0;
  virtual RenderState::DepthLayer GetDepthLayer() const = 0;
  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color const & GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetDepth(size_t layerIndex) const = 0;
  virtual std::vector<m2::PointD> const & GetPoints() const = 0;

private:
  LineID m_id;
};

class UserMarksProvider
{
public:
  virtual ~UserMarksProvider() {}
  virtual GroupIDSet const & GetDirtyGroupIds() const = 0;
  virtual GroupIDSet GetAllGroupIds() const = 0;
  virtual bool IsGroupVisible(MarkGroupID groupID) const = 0;
  virtual bool IsGroupVisibilityChanged(MarkGroupID groupID) const = 0;
  virtual MarkIDSet const & GetGroupPointIds(MarkGroupID groupID) const = 0;
  virtual LineIDSet const & GetGroupLineIds(MarkGroupID groupID) const = 0;
  virtual MarkIDSet const & GetCreatedMarkIds() const = 0;
  virtual MarkIDSet const & GetRemovedMarkIds() const = 0;
  virtual MarkIDSet const & GetUpdatedMarkIds() const = 0;
  /// Never store UserPointMark reference.
  virtual UserPointMark const * GetUserPointMark(MarkID markID) const = 0;
  /// Never store UserLineMark reference.
  virtual UserLineMark const * GetUserLineMark(LineID lineID) const = 0;
};

} // namespace df
