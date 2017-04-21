#pragma once
#include "search/ranking_info.hpp"

#include "indexer/feature_decl.hpp"

#include "editor/yes_no_unknown.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/buffer_vector.hpp"

#include "std/string.hpp"


namespace search
{
// Search result. Search returns a list of them, ordered by score.
class Result
{
public:
  enum ResultType
  {
    RESULT_FEATURE,
    RESULT_LATLON,
    RESULT_SUGGEST_PURE,
    RESULT_SUGGEST_FROM_FEATURE
  };

  /// Metadata for search results. Considered valid if m_resultType == RESULT_FEATURE.
  struct Metadata
  {
    string m_cuisine;                              // Valid only if not empty. Used for restaurants.

    // Following fields are used for hotels only.
    string m_hotelApproximatePricing;
    string m_hotelRating;
    int m_stars = 0;
    bool m_isSponsoredHotel = false;
    bool m_isHotel = false;

    osm::YesNoUnknown m_isOpenNow = osm::Unknown;  // Valid for any result.

    bool m_isInitialized = false;
  };

  /// For RESULT_FEATURE.
  Result(FeatureID const & id, m2::PointD const & pt, string const & str, string const & address,
         string const & type, uint32_t featureType, Metadata const & meta);

  /// For RESULT_LATLON.
  Result(m2::PointD const & pt, string const & latlon, string const & address);

  /// For RESULT_SUGGESTION_PURE.
  Result(string const & str, string const & suggest);

  /// For RESULT_SUGGESTION_FROM_FEATURE.
  Result(Result const & res, string const & suggest);

  /// Strings that is displayed in the GUI.
  //@{
  string const & GetString() const { return m_str; }
  string const & GetAddress() const { return m_address; }
  string const & GetFeatureType() const { return m_type; }
  string const & GetCuisine() const { return m_metadata.m_cuisine; }
  string const & GetHotelRating() const { return m_metadata.m_hotelRating; }
  string const & GetHotelApproximatePricing() const { return m_metadata.m_hotelApproximatePricing; }
  bool IsHotel() const { return m_metadata.m_isHotel; }
  //@}

  osm::YesNoUnknown IsOpenNow() const { return m_metadata.m_isOpenNow; }
  int GetStarsCount() const { return m_metadata.m_stars; }

  bool IsSuggest() const;
  bool HasPoint() const;

  /// Type of the result.
  ResultType GetResultType() const;

  /// Feature id in mwm.
  /// @precondition GetResultType() == RESULT_FEATURE
  FeatureID const & GetFeatureID() const;

  /// Center point of a feature.
  /// @precondition HasPoint() == true
  m2::PointD GetFeatureCenter() const;

  /// String to write in the search box.
  /// @precondition IsSuggest() == true
  char const * GetSuggestionString() const;

  bool IsEqualSuggest(Result const & r) const;
  bool IsEqualFeature(Result const & r) const;

  void AddHighlightRange(pair<uint16_t, uint16_t> const & range);
  pair<uint16_t, uint16_t> const & GetHighlightRange(size_t idx) const;
  inline size_t GetHighlightRangesCount() const { return m_hightlightRanges.size(); }

  void AppendCity(string const & name);

  int32_t GetPositionInResults() const { return m_positionInResults; }
  void SetPositionInResults(int32_t pos) { m_positionInResults = pos; }

  inline RankingInfo const & GetRankingInfo() const { return m_info; }

  template <typename TInfo>
  inline void SetRankingInfo(TInfo && info)
  {
    m_info = forward<TInfo>(info);
  }

  // Returns a representation of this result that is
  // sent to the statistics servers and later used to measure
  // the quality of our search engine.
  string ToStringForStats() const;

private:
  FeatureID m_id;
  m2::PointD m_center;
  string m_str, m_address, m_type;
  uint32_t m_featureType;
  string m_suggestionStr;
  buffer_vector<pair<uint16_t, uint16_t>, 4> m_hightlightRanges;

  RankingInfo m_info;

  // The position that this result occupied in the vector returned
  // by a search query. -1 if undefined.
  int32_t m_positionInResults = -1;

public:
  Metadata m_metadata;
};

class Results
{
public:
  using Iter = vector<Result>::iterator;
  using ConstIter = vector<Result>::const_iterator;

  Results();

  inline bool IsEndMarker() const { return m_status != Status::None; }
  inline bool IsEndedNormal() const { return m_status == Status::EndedNormal; }
  inline bool IsEndedCancelled() const { return m_status == Status::EndedCancelled; }

  void SetEndMarker(bool cancelled)
  {
    m_status = cancelled ? Status::EndedCancelled : Status::EndedNormal;
  }

  bool AddResult(Result && result);

  // Fast version of AddResult() that doesn't do any duplicates checks.
  void AddResultNoChecks(Result && result);
  void AddResultsNoChecks(ConstIter first, ConstIter last);

  void Clear();

  inline Iter begin() { return m_results.begin(); }
  inline Iter end() { return m_results.end(); }
  inline ConstIter begin() const { return m_results.cbegin(); }
  inline ConstIter end() const { return m_results.cend(); }

  inline size_t GetCount() const { return m_results.size(); }
  size_t GetSuggestsCount() const;

  inline Result & operator[](size_t i)
  {
    ASSERT_LESS(i, m_results.size(), ());
    return m_results[i];
  }

  inline Result const & operator[](size_t i) const
  {
    ASSERT_LESS(i, m_results.size(), ());
    return m_results[i];
  }

  inline void Swap(Results & rhs) { m_results.swap(rhs.m_results); }

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
  void InsertResult(vector<Result>::iterator where, Result && result);

  vector<Result> m_results;
  Status m_status;
};

struct AddressInfo
{
  string m_country, m_city, m_street, m_house, m_name;
  vector<string> m_types;
  double m_distanceMeters = -1.0;

  string GetPinName() const;    // Caroline
  string GetPinType() const;    // shop

  string FormatPinText() const; // Caroline (clothes shop)
  string FormatTypes() const;   // clothes shop
  string GetBestType() const;
  bool IsEmptyName() const;

  enum AddressType { DEFAULT, SEARCH_RESULT };
  // 7 vulica Frunze
  string FormatHouseAndStreet(AddressType type = DEFAULT) const;
  // 7 vulica Frunze, Minsk, Belarus
  string FormatAddress(AddressType type = DEFAULT) const;
  // Caroline, 7 vulica Frunze, Minsk, Belarus
  string FormatNameAndAddress(AddressType type = DEFAULT) const;

  void Clear();

  friend string DebugPrint(AddressInfo const & info);
};

string DebugPrint(search::Result const & result);
}  // namespace search
