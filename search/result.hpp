#pragma once

#include "search/bookmarks/results.hpp"
#include "search/hotels_classifier.hpp"
#include "search/ranking_info.hpp"

#include "indexer/feature_decl.hpp"

#include "editor/yes_no_unknown.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/buffer_vector.hpp"

#include "defines.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace search
{
// Search result. Search returns a list of them, ordered by score.
//
// TODO (@y, @m): split this class to small classes by type, replace
// Results::m_results by vectors corresponding to types.
class Result
{
public:
  enum class Type
  {
    Feature,
    LatLon,
    PureSuggest,
    SuggestFromFeature
  };

  // Metadata for search results. Considered valid if GetResultType() == Type::Feature.
  struct Metadata
  {
    // Valid only if not empty, used for restaurants.
    std::string m_cuisine;

    // Valid only if not empty, used for airport iata codes.
    std::string m_airportIata;

    // Valid only if not empty, used for brand name.
    std::string m_brand;

    // Following fields are used for hotels only.
    int m_hotelPricing = 0;
    std::string m_hotelApproximatePricing;
    float m_hotelRating = kInvalidRatingValue;
    int m_stars = 0;
    bool m_isSponsoredHotel = false;
    bool m_isHotel = false;

    // Valid for any result.
    osm::YesNoUnknown m_isOpenNow = osm::Unknown;

    bool m_isInitialized = false;
  };

  // Min distance to search result when popularity label has a higher priority (in meters).
  static auto constexpr kPopularityHighPriorityMinDistance = 50000.0;

  // For Type::Feature.
  Result(FeatureID const & id, m2::PointD const & pt, std::string const & str,
         std::string const & address, uint32_t featureType, Metadata const & meta);

  // For Type::LatLon.
  Result(m2::PointD const & pt, std::string const & latlon, std::string const & address);

  // For Type::PureSuggest.
  Result(std::string const & str, std::string const & suggest);

  // For Type::SuggestFromFeature.
  Result(Result const & res, std::string const & suggest);

  Type GetResultType() const { return m_resultType; }

  std::string const & GetString() const { return m_str; }
  std::string const & GetAddress() const { return m_address; }
  std::string const & GetCuisine() const { return m_metadata.m_cuisine; }
  std::string const & GetAirportIata() const { return m_metadata.m_airportIata; }
  std::string const & GetBrand() const { return m_metadata.m_brand; }
  float GetHotelRating() const { return m_metadata.m_hotelRating; }
  std::string const & GetHotelApproximatePricing() const
  {
    return m_metadata.m_hotelApproximatePricing;
  }
  bool IsHotel() const { return m_metadata.m_isHotel; }

  osm::YesNoUnknown IsOpenNow() const { return m_metadata.m_isOpenNow; }
  int GetStarsCount() const { return m_metadata.m_stars; }

  bool IsSuggest() const;
  bool HasPoint() const;

  // Feature id in mwm.
  // Precondition: GetResultType() == Type::Feature.
  FeatureID const & GetFeatureID() const;

  // Precondition: GetResultType() == Type::Feature.
  uint32_t GetFeatureType() const;

  // Center point of a feature.
  // Precondition: HasPoint() == true.
  m2::PointD GetFeatureCenter() const;

  // String to write in the search box.
  // Precondition: IsSuggest() == true.
  std::string const & GetSuggestionString() const;

  bool IsEqualSuggest(Result const & r) const;
  bool IsEqualFeature(Result const & r) const;

  void AddHighlightRange(std::pair<uint16_t, uint16_t> const & range);
  std::pair<uint16_t, uint16_t> const & GetHighlightRange(size_t idx) const;
  size_t GetHighlightRangesCount() const { return m_hightlightRanges.size(); }

  void PrependCity(std::string const & name);

  int32_t GetPositionInResults() const { return m_positionInResults; }
  void SetPositionInResults(int32_t pos) { m_positionInResults = pos; }

  RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename Info>
  void SetRankingInfo(Info && info)
  {
    m_info = forward<Info>(info);
  }

  // Returns a representation of this result that is sent to the
  // statistics servers and later used to measure the quality of our
  // search engine.
  std::string ToStringForStats() const;

private:
  Type m_resultType;

  FeatureID m_id;
  m2::PointD m_center;
  std::string m_str;
  std::string m_address;
  uint32_t m_featureType = 0;
  std::string m_suggestionStr;
  buffer_vector<std::pair<uint16_t, uint16_t>, 4> m_hightlightRanges;

  RankingInfo m_info;

  // The position that this result occupied in the vector returned by
  // a search query. -1 if undefined.
  int32_t m_positionInResults = -1;

public:
  Metadata m_metadata;
};

std::string DebugPrint(search::Result::Type type);
std::string DebugPrint(search::Result const & result);

class Results
{
public:
  using Iter = std::vector<Result>::iterator;
  using ConstIter = std::vector<Result>::const_iterator;

  enum class Type
  {
    Default,
    Hotels
  };

  Results();

  bool IsEndMarker() const { return m_status != Status::None; }
  bool IsEndedNormal() const { return m_status == Status::EndedNormal; }
  bool IsEndedCancelled() const { return m_status == Status::EndedCancelled; }

  void SetEndMarker(bool cancelled)
  {
    m_status = cancelled ? Status::EndedCancelled : Status::EndedNormal;
  }

  bool AddResult(Result && result);

  // Fast version of AddResult() that doesn't do any checks for
  // duplicates.
  void AddResultNoChecks(Result && result);
  void AddResultsNoChecks(ConstIter first, ConstIter last);

  void AddBookmarkResult(bookmarks::Result const & result);

  void Clear();

  Iter begin() { return m_results.begin(); }
  Iter end() { return m_results.end(); }
  ConstIter begin() const { return m_results.cbegin(); }
  ConstIter end() const { return m_results.cend(); }

  size_t GetCount() const { return m_results.size(); }
  size_t GetSuggestsCount() const;

  Result & operator[](size_t i)
  {
    ASSERT_LESS(i, m_results.size(), ());
    return m_results[i];
  }

  Result const & operator[](size_t i) const
  {
    ASSERT_LESS(i, m_results.size(), ());
    return m_results[i];
  }

  bookmarks::Results const & GetBookmarksResults() const;

  template <typename Fn>
  void SortBy(Fn && comparator)
  {
    sort(begin(), end(), std::forward<Fn>(comparator));
    for (int32_t i = 0; i < static_cast<int32_t>(GetCount()); ++i)
      operator[](i).SetPositionInResults(i);
  }

  Type GetType() const
  {
    return m_hotelsClassif.IsHotelResults() ? Type::Hotels : Type::Default;
  }

private:
  enum class Status
  {
    None,
    EndedCancelled,
    EndedNormal
  };

  // Inserts |result| in |m_results| at position denoted by |where|.
  //
  // *NOTE* all iterators, references and pointers to |m_results| are
  // invalid after the call.
  void InsertResult(std::vector<Result>::iterator where, Result && result);

  std::vector<Result> m_results;
  bookmarks::Results m_bookmarksResults;
  Status m_status;
  HotelsClassifier m_hotelsClassif;
};

std::string DebugPrint(search::Results const & results);

struct AddressInfo
{
  enum class Type { Default, SearchResult };

  std::string m_country;
  std::string m_city;
  std::string m_street;
  std::string m_house;
  std::string m_name;
  std::vector<std::string> m_types;
  double m_distanceMeters = -1.0;

  std::string GetPinName() const;    // Caroline
  std::string GetPinType() const;    // shop

  std::string FormatPinText() const; // Caroline (clothes shop)
  std::string FormatTypes() const;   // clothes shop
  std::string GetBestType() const;
  bool IsEmptyName() const;

  // 7 vulica Frunze
  std::string FormatHouseAndStreet(Type type = Type::Default) const;

  // 7 vulica Frunze, Minsk, Belarus
  std::string FormatAddress(Type type = Type::Default) const;

  // Caroline, 7 vulica Frunze, Minsk, Belarus
  std::string FormatNameAndAddress(Type type = Type::Default) const;

  void Clear();
};

std::string DebugPrint(AddressInfo const & info);
}  // namespace search
