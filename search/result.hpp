#pragma once
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
    RESULT_SUGGESTION
  };

  Result(string const & str, uint32_t featureType, m2::RectD const & featureRect, double distance);
  Result(string const & str, string const & suggestionStr);

  // String that is displayed in the GUI.
  string GetString() const { return m_str; }

  // Type of the result.
  ResultType GetResultType() const;

  // Rect of a feature, if GetResultType() == RESULT_FEATURE.
  m2::RectD GetFeatureRect() const;

  // Type of a feature, if GetResultType() == RESULT_FEATURE.
  uint32_t GetFetureType() const;

  // Distance to the center of the screen, if GetResultType() == RESULT_FEATURE.
  double GetDistanceToCenter() const;

  // String writo in the search box.
  string GetSuggestionString() const;

private:
  string m_str;
  m2::RectD m_featureRect;
  uint32_t m_featureType;
  double m_disance;
  string m_suggestionStr;
};

}
