#pragma once

#include "drape_frontend/render_state.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/drape_global.hpp"
#include "drape/pointers.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/polyline2d.hpp"

#include <vector>

namespace df
{
using MarkID = uint32_t;
using IDCollection = std::vector<MarkID>;

using MarkGroupID = size_t;

struct MarkIDCollection
{
  IDCollection m_marksID;
  IDCollection m_linesID;

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

  UserPointMark();
  virtual ~UserPointMark() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges() const = 0;

  MarkID GetId() const { return m_id; }

  virtual m2::PointD const & GetPivot() const = 0;
  virtual m2::PointD GetPixelOffset() const = 0;
  virtual dp::Anchor GetAnchor() const = 0;
  virtual float GetDepth() const = 0;
  virtual RenderState::DepthLayer GetDepthLayer() const = 0;
  virtual bool IsVisible() const = 0;
  virtual drape_ptr<TitlesInfo> GetTitleDecl() const = 0;
  virtual drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const = 0;
  virtual drape_ptr<ColoredSymbolZoomInfo> GetColoredSymbols() const = 0;
  virtual uint16_t GetPriority() const = 0;
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
  UserLineMark();
  virtual ~UserLineMark() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges() const = 0;

  virtual MarkID GetId() const { return m_id; }

  virtual int GetMinZoom() const = 0;
  virtual RenderState::DepthLayer GetDepthLayer() const = 0;
  virtual size_t GetLayerCount() const = 0;
  virtual dp::Color const & GetColor(size_t layerIndex) const = 0;
  virtual float GetWidth(size_t layerIndex) const = 0;
  virtual float GetDepth(size_t layerIndex) const = 0;
  virtual std::vector<m2::PointD> const & GetPoints() const = 0;

private:
  MarkID m_id;
};

class UserMarksProvider
{
public:
  UserMarksProvider();
  virtual ~UserMarksProvider() {}

  virtual bool IsDirty() const = 0;
  virtual void AcceptChanges(MarkIDCollection & createdMarks, MarkIDCollection & removedMarks) = 0;

  virtual bool IsDrawable() const = 0;

  virtual size_t GetUserPointCount() const = 0;
  /// never store UserPointMark reference
  virtual UserPointMark const * GetUserPointMark(size_t index) const = 0;

  virtual size_t GetUserLineCount() const = 0;
  /// never store UserLineMark reference
  virtual UserLineMark const * GetUserLineMark(size_t index) const = 0;

  bool IsPendingOnDelete();
  void DeleteLater();

private:
  bool m_pendingOnDelete;
};

} // namespace df
