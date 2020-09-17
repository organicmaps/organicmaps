#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/control_flow.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <mutex>

class BookmarkManager;

class SearchMarkPoint : public UserMark
{
public:
  enum class SearchMarkType : uint32_t;

  explicit SearchMarkPoint(m2::PointD const & ptOrg);

  m2::PointD GetPixelOffset() const override;
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
  bool IsSymbolSelectable() const override { return true; }
  bool IsNonDisplaceable() const override { return true; }

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetFromType(uint32_t type, bool hasLocalAds);
  void SetBookingType(bool hasLocalAds);
  void SetHotelType(bool hasLocalAds);
  void SetNotFoundType();

  void SetPreparing(bool isPreparing);
  void SetRating(float rating);
  void SetPricing(int pricing);
  void SetPrice(std::string && price);
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

  bool IsBookingSpecialMark() const;
  bool IsHotel() const;

  bool HasRating() const;
  bool HasGoodRating() const;
  bool HasPrice() const;
  bool HasPricing() const;
  bool HasReason() const;

  std::string GetSymbolName() const;
  std::string GetBadgeName() const;

  SearchMarkType m_type{};
  bool m_hasLocalAds = false;
  FeatureID m_featureID;
  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  bool m_isPreparing = false;
  float m_rating = 0.0f;
  int m_pricing = 0;
  std::string m_price;
  bool m_hasSale = false;
  bool m_isSelected = false;
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

  m2::PointD GetMaxDimension(ScreenBase const & modelView) const;

  // NOTE: Vector of features must be sorted.
  void SetPreparingState(std::vector<FeatureID> const & features, bool isPreparing);

  // NOTE: Vector of features must be sorted.
  void SetSales(std::vector<FeatureID> const & features, bool hasSale);

  // NOTE: Vector of features must be sorted.
  void SetPrices(std::vector<FeatureID> const & features, std::vector<std::string> && prices);

  bool IsThereSearchMarkForFeature(FeatureID const & featureId) const;
  void OnActivate(FeatureID const & featureId);
  void OnDeactivate(FeatureID const & featureId);

  void SetUnavailable(SearchMarkPoint & mark, std::string const & reasonKey);
  void SetUnavailable(std::vector<FeatureID> const & features, std::string const & reasonKey);
  bool IsUnavailable(FeatureID const & id) const;

  void SetVisited(FeatureID const & id);
  bool IsVisited(FeatureID const & id) const;

  void SetSelected(FeatureID const & id);
  bool IsSelected(FeatureID const & id) const;

  void ClearTrackedProperties();

  static bool HaveSizes() { return !m_searchMarkSizes.empty(); };
  static std::optional<m2::PointD> GetSize(std::string const & symbolName);

private:
  void ProcessMarks(std::function<base::ControlFlow(SearchMarkPoint *)> && processor) const;
  void UpdateMaxDimension();

  BookmarkManager * m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;

  static std::map<std::string, m2::PointF> m_searchMarkSizes;

  m2::PointD m_maxDimension;

  std::set<FeatureID> m_visitedSearchMarks;
  FeatureID m_selectedFeature;

  mutable std::mutex m_lock;
  std::map<FeatureID, std::string /* SearchMarkPoint::m_reason */> m_unavailable;
};
