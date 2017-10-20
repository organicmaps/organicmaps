#pragma once

#include "map/user_mark.hpp"

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

  static std::vector<std::string> const & GetAllSymbolsNames();
  static void SetSearchMarksSizes(std::vector<m2::PointF> const & sizes);
  static double GetMaxSearchMarkDimension(ScreenBase const & modelView);

protected:
  static m2::PointD GetSearchMarkSize(SearchMarkType searchMarkType,
                                      ScreenBase const & modelView);

  SearchMarkType m_type = SearchMarkType::Default;
  FeatureID m_featureID;
  // Used to pass exact search result matched string into a place page.
  std::string m_matchedName;
  bool m_isPreparing = false;

  static std::vector<m2::PointF> m_searchMarksSizes;
};
