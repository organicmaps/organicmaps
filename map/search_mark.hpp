#pragma once

#include "map/user_mark.hpp"

#include "drape_frontend/drape_engine_safe_ptr.hpp"

#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"

#include <string>
#include <vector>

enum class SearchMarkType
{
  Default = 0,
  Booking,
  LocalAds,
  Cian, // TODO: delete me after Cian project is finished.

  NotFound, // Service value used in developer tools.
  Count
};

class BookmarkManager;

class SearchMarkPoint : public UserMark
{
public:
  SearchMarkPoint(m2::PointD const & ptOrg, UserMarkContainer * container);

  std::string GetSymbolName() const override;
  UserMark::Type GetMarkType() const override;

  FeatureID GetFeatureID() const override { return m_featureID; }
  void SetFoundFeature(FeatureID const & feature);

  std::string const & GetMatchedName() const { return m_matchedName; }
  void SetMatchedName(std::string const & name);

  void SetMarkType(SearchMarkType type);

  void SetPreparing(bool isPreparing);

protected:
  template<typename T> void SetAttributeValue(T & dst, T const & src)
  {
    if (dst == src)
      return;

    SetDirty();
    dst = src;
  }

  SearchMarkType m_type = SearchMarkType::Default;
  FeatureID m_featureID;
  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  bool m_isPreparing = false;
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

private:
  m2::PointD GetSize(SearchMarkType searchMarkType, ScreenBase const & modelView) const;

  BookmarkManager * m_bmManager;
  df::DrapeEngineSafePtr m_drapeEngine;

  std::vector<m2::PointF> m_searchMarksSizes;
};
