#pragma once
#include "../indexer/feature_decl.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"

#include "../std/string.hpp"


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
    RESULT_SUGGESTION,
    RESULT_POI_SUGGEST
  };

  /// For RESULT_FEATURE.
  Result(FeatureID const & id, m2::PointD const & fCenter,
         string const & str, string const & region,
         string const & flag, string const & type,
         uint32_t featureType, double distance);

  /// For RESULT_LATLON.
  Result(m2::PointD const & fCenter,
         string const & str, string const & region,
         string const & flag, double distance);

  /// For RESULT_SUGGESTION.
  Result(string const & str, string const & suggest);

  /// Strings that is displayed in the GUI.
  //@{
  char const * GetString() const { return m_str.c_str(); }
  char const * GetRegionString() const { return m_region.c_str(); }
  char const * GetRegionFlag() const { return m_flag.empty() ? 0 : m_flag.c_str(); }
  char const * GetFeatureType() const { return m_type.c_str(); }
  //@}

  /// Type of the result.
  ResultType GetResultType() const;

  /// Feature id in mwm.
  /// @precondition GetResultType() == RESULT_FEATURE
  FeatureID GetFeatureID() const;

  /// Center point of a feature.
  /// @precondition GetResultType() != RESULT_SUGGESTION
  m2::PointD GetFeatureCenter() const;

  /// Distance from the current position or -1 if location is not detected.
  /// @precondition GetResultType() != RESULT_SUGGESTION
  double GetDistance() const;

  /// String to write in the search box.
  /// @precondition GetResultType() == RESULT_SUGGESTION
  char const * GetSuggestionString() const;

  bool operator== (Result const & r) const;

private:
  FeatureID m_id;
  m2::PointD m_center;
  string m_str, m_region, m_flag, m_type;
  uint32_t m_featureType;
  double m_distance;
  string m_suggestionStr;
};

class Results
{
  vector<Result> m_vec;

  enum StatusT {
    NONE,             // default status
    ENDED_CANCELLED,  // search ended with canceling
    ENDED             // search ended itself
  };
  StatusT m_status;

  explicit Results(bool isCancelled)
  {
    m_status = (isCancelled ? ENDED_CANCELLED : ENDED);
  }

public:
  Results() : m_status(NONE) {}

  /// @name To implement end of search notification.
  //@{
  static Results GetEndMarker(bool isCancelled) { return Results(isCancelled); }
  bool IsEndMarker() const { return (m_status != NONE); }
  bool IsEndedNormal() const { return (m_status == ENDED); }
  //@}

  inline bool AddResult(Result const & r)
  {
    m_vec.push_back(r);
    return true;
  }
  bool AddResultCheckExisting(Result const & r);

  inline void Clear() { m_vec.clear(); }

  typedef vector<Result>::const_iterator IterT;
  inline IterT Begin() const { return m_vec.begin(); }
  inline IterT End() const { return m_vec.end(); }

  inline size_t GetCount() const { return m_vec.size(); }

  inline Result const & GetResult(size_t i) const
  {
    ASSERT_LESS(i, m_vec.size(), ());
    return m_vec[i];
  }

  inline void Swap(Results & rhs)
  {
    m_vec.swap(rhs.m_vec);
  }

  template <class LessT> void Sort(LessT lessFn)
  {
    sort(m_vec.begin(), m_vec.end(), lessFn);
  }
};

struct AddressInfo
{
  string m_country, m_city, m_street, m_house, m_name;
  vector<string> m_types;

  void MakeFrom(search::Result const & res);

  void SetPinName(string const & name);
  string GetPinName() const;
  string GetPinType() const;

  string FormatPinText() const;
  string FormatAddress() const;
  string FormatTypes() const;
  string FormatNameAndAddress() const;

  char const * GetBestType() const;

  void Clear();
};

}
