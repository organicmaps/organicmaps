#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "std/string.hpp"

#include "3party/jansson/myjansson.hpp"

namespace search
{
struct Sample
{
  struct Result
  {
    enum Relevance
    {
      RELEVANCE_IRRELEVANT,
      RELEVANCE_RELEVANT,
      RELEVANCE_VITAL
    };

    bool operator<(Result const & rhs) const;

    bool operator==(Result const & rhs) const;

    m2::PointD m_pos = m2::PointD(0, 0);
    strings::UniString m_name;
    string m_houseNumber;
    vector<string> m_types;  // MAPS.ME types, not OSM types.
    Relevance m_relevance = RELEVANCE_IRRELEVANT;
  };

  bool DeserializeFromJSON(string const & jsonStr);

  static bool DeserializeFromJSON(string const & jsonStr, vector<Sample> & samples);

  bool operator<(Sample const & rhs) const;

  bool operator==(Sample const & rhs) const;

  void DeserializeFromJSONImpl(json_t * root);

  strings::UniString m_query;
  string m_locale;
  m2::PointD m_pos = m2::PointD(0, 0);
  m2::RectD m_viewport = m2::RectD(0, 0, 0, 0);
  vector<Result> m_results;
};

string DebugPrint(Sample::Result::Relevance r);

string DebugPrint(Sample::Result const & r);

string DebugPrint(Sample const & s);
}  // namespace search
