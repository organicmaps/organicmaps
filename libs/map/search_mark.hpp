#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/control_flow.hpp"

#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <vector>

class BookmarkManager;

class SearchMarkPoint : public UserMark
{
public:
  enum SearchMarkType : uint8_t;

  explicit SearchMarkPoint(m2::PointD const & ptOrg);

  m2::PointD GetPixelOffset() const override;
  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  df::ColorConstant GetColorConstant() const override;
  drape_ptr<TitlesInfo> GetTitleDecl() const override;
  int GetMinTitleZoom() const override;
  df::DepthLayer GetDepthLayer() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  bool GetDepthTestEnabled() const override { return false; }
  bool IsMarkAboveText() const override;
  float GetSymbolOpacity() const override;
  bool IsSymbolSelectable() const override { return true; }
  bool IsNonDisplaceable() const override { return true; }

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetFromType(uint32_t type);
  void SetNotFoundType();

  void SetPreparing(bool isPreparing);
  void SetSale(bool hasSale);
  void SetSelected(bool isSelected);
  void SetVisited(bool isVisited);
  void SetAvailable(bool isAvailable);
  void SetReason(std::string const & reason);

  bool IsSelected() const;
  bool IsAvailable() const;
  std::string const & GetReason() const;

protected:
  template <typename T, typename U>
  void SetAttributeValue(T & dst, U && src)
  {
    if (dst == src)
      return;

    SetDirty();
    dst = std::forward<U>(src);
  }

  bool HasReason() const;

  std::string const * GetSymbolName() const;

  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  std::string m_reason;

  FeatureID m_featureID;
  SearchMarkType m_type;

  bool m_isPreparing : 1;
  bool m_hasSale : 1;
  bool m_isSelected : 1;
  bool m_isVisited : 1;
  bool m_isAvailable : 1;
};

class SearchMarks
{
public:
  SearchMarks();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetBookmarkManager(BookmarkManager * bmManager);

  m2::PointD GetMaxDimension(ScreenBase const & modelView) const;

  // NOTE: Vector of features must be sorted.
  void SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing);

  // NOTE: Vector of features must be sorted.
  void SetSales(std::vector<FeatureID> const & features, bool hasSale);

  bool IsThereSearchMarkForFeature(FeatureID const & featureId) const;
  void OnActivate(FeatureID const & featureId);
  void OnDeactivate(FeatureID const & featureId);

  //  void SetUnavailable(SearchMarkPoint & mark, std::string const & reasonKey);
  //  void SetUnavailable(std::vector<FeatureID> const & features, std::string const & reasonKey);
  //  bool IsUnavailable(FeatureID const & id) const;

  void SetVisited(FeatureID const & id);
  bool IsVisited(FeatureID const & id) const;

  void SetSelected(FeatureID const & id);
  bool IsSelected(FeatureID const & id) const;

  void ClearTrackedProperties();

  static bool HaveSizes() { return !s_markSizes.empty(); }
  static std::optional<m2::PointD> GetSize(std::string const & symbolName);

private:
  void ProcessMarks(std::function<base::ControlFlow(SearchMarkPoint *)> && processor) const;
  void UpdateMaxDimension();

  BookmarkManager * m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;

  static std::map<std::string, m2::PointF> s_markSizes;

  m2::PointD m_maxDimension{0, 0};

  std::set<FeatureID> m_visitedSearchMarks;
  FeatureID m_selectedFeature;

  //  mutable std::mutex m_lock;
  //  std::map<FeatureID, std::string /* SearchMarkPoint::m_reason */> m_unavailable;
};
