#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include <boost/optional.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>

enum class SearchMarkType
{
  Default = 0,
  Booking,
  LocalAds,
  LocalAdsBooking,
  UGC,
  NotFound, // Service value used in developer tools.
  Count
};

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
  drape_ptr<SymbolNameZoomInfo> GetBadgeNames() const override;
  drape_ptr<SymbolOffsets> GetSymbolOffsets() const override;
  bool GetDepthTestEnabled() const override { return false; }

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetMarkType(SearchMarkType type);
  void SetPreparing(bool isPreparing);
  void SetRating(float rating);
  void SetPricing(int pricing);
  void SetSale(bool hasSale);

protected:
  template<typename T> void SetAttributeValue(T & dst, T const & src)
  {
    if (dst == src)
      return;

    SetDirty();
    dst = src;
  }

  bool IsBookingSpecialMark() const;
  bool IsUGCMark() const;
  bool IsMarkWithRating() const;

  SearchMarkType m_type = SearchMarkType::Default;
  FeatureID m_featureID;
  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  bool m_isPreparing = false;
  float m_rating = 0.0f;
  int m_pricing = 0;
  bool m_hasSale = false;
  dp::TitleDecl m_titleDecl;
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

  static m2::PointD GetSize(SearchMarkType searchMarkType, ScreenBase const & modelView);
  static boost::optional<m2::PointD> GetSize(std::string const & symbolName);

private:
  void FilterAndProcessMarks(std::vector<FeatureID> const & features,
                             std::function<void(SearchMarkPoint *)> && processor);

  BookmarkManager * m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;

  static std::map<std::string, m2::PointF> m_searchMarksSizes;
};
