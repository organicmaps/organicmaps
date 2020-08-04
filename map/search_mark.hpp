#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class BookmarkManager;

class SearchMarkPoint : public UserMark
{
public:
  explicit SearchMarkPoint(m2::PointD const & ptOrg);

  drape_ptr<SymbolNameZoomInfo> GetSymbolNames() const override;
  df::ColorConstant GetColorConstant() const override;
  drape_ptr<TitlesInfo> GetTitleDecl() const override;
  int GetMinTitleZoom() const override;
  df::DepthLayer GetDepthLayer() const override;
  drape_ptr<BageInfo> GetBadgeInfo() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  bool GetDepthTestEnabled() const override { return false; }
  bool IsMarkAboveText() const override;
  float GetSymbolOpacity() const override;

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetFromType(uint32_t type, bool hasLocalAds);
  void SetBookingType(bool hasLocalAds);
  void SetNotFoundType();

  void SetPreparing(bool isPreparing);
  void SetRating(float rating);
  void SetPricing(int pricing);
  void SetPrice(std::string && price);
  void SetSale(bool hasSale);
  void SetVisited(bool isVisited);
  void SetAvailable(bool isAvailable);
  void SetReason(std::string const & reason);

protected:
  template <typename T, typename U>
  void SetAttributeValue(T & dst, U && src)
  {
    if (dst == src)
      return;

    SetDirty();
    dst = std::forward<U>(src);
  }

  bool IsBookingSpecialMark() const;
  bool HasGoodRating() const;
  bool HasPrice() const;
  bool HasPricing() const;
  bool HasRating() const;
  bool IsUGCMark() const;
  bool IsSelected() const;
  bool HasSale() const;
  std::string GetSymbolName() const;
  std::string GetBadgeName() const;

  uint8_t m_type = 0;
  bool m_hasLocalAds = false;
  FeatureID m_featureID;
  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  bool m_isPreparing = false;
  float m_rating = 0.0f;
  int m_pricing = 0;
  std::string m_price;
  bool m_hasSale = false;
  dp::TitleDecl m_titleDecl;
  dp::TitleDecl m_badgeTitleDecl;
  dp::TitleDecl m_ugcTitleDecl;
  bool m_isVisited = false;
  bool m_isAvailable = true;
  std::string m_reason;
};

class SearchMarks
{
public:
  SearchMarks();

  void SetDrapeEngine(ref_ptr<df::DrapeEngine> engine);
  void SetBookmarkManager(BookmarkManager * bmManager);

  double GetMaxDimension(ScreenBase const & modelView) const;

  // NOTE: Vector of features must be sorted.
  void SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing);

  // NOTE: Vector of features must be sorted.
  void SetSales(std::vector<FeatureID> const & features, bool hasSale);

  // NOTE: Vector of features must be sorted.
  void SetPrices(std::vector<FeatureID> const & features, std::vector<std::string> && prices);

  void OnActivate(FeatureID const & featureId);
  void OnDeactivate(FeatureID const & featureId);

  bool IsVisited(FeatureID const & id) const;
  void ClearVisited();

  void SetUnavailable(FeatureID const & id, std::string const & reason);
  bool IsUnavailable(FeatureID const & id) const;
  void MarkUnavailableIfNeeded(SearchMarkPoint * mark) const;
  void ClearUnavailable();

  static bool HaveSizes() { return !m_searchMarksSizes.empty(); };
  static std::optional<m2::PointD> GetSize(std::string const & symbolName);

private:
  void ProcessMarks(std::function<void(SearchMarkPoint *)> && processor);

  BookmarkManager * m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;

  static std::map<std::string, m2::PointF> m_searchMarksSizes;

  std::set<FeatureID> m_visited;
  // The value is localized string key for unavailability reason.
  std::map<FeatureID, std::string> m_unavailable;
};
