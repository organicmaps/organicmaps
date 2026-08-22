#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/control_flow.hpp"

#include <functional>
#include <map>
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
  int GetMinTitleZoom() const override;
  df::DepthLayer GetDepthLayer() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  bool GetDepthTestEnabled() const override { return false; }
  bool IsMarkAboveText() const override;
  float GetSymbolOpacity() const override;
  bool IsSymbolSelectable() const override { return true; }

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetFromType(uint32_t type);
  void SetNotFoundType();

  void SetVisited(bool isVisited);

protected:
  template <typename T, typename U>
  void SetAttributeValue(T & dst, U && src)
  {
    if (dst == src)
      return;

    SetDirty();
    dst = std::forward<U>(src);
  }

  std::string const * GetSymbolName() const;

  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;

  FeatureID m_featureID;
  SearchMarkType m_type;

  bool m_isVisited : 1;
};

class SearchMarks
{
public:
  SearchMarks();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetBookmarkManager(BookmarkManager * bmManager);

  m2::PointD GetMaxDimension(ScreenBase const & modelView) const;

  bool IsThereSearchMarkForFeature(FeatureID const & featureId) const;
  void OnDeactivate(FeatureID const & featureId);

  void SetVisited(FeatureID const & id);
  bool IsVisited(FeatureID const & id) const;

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
};
