#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "std/string.hpp"

#include "3party/jansson/myjansson.hpp"

namespace search
{
class Sample
{
public:
  struct Result
  {
    enum Relevance
    {
      RELEVANCE_IRRELEVANT,
      RELEVANCE_RELEVANT,
      RELEVANCE_VITAL
    };

    m2::PointD m_pos;
    strings::UniString m_name;
    string m_houseNumber;
    vector<string> m_types;  // MAPS.ME types, not OSM types.
    Relevance m_relevance;
  };

  bool DeserializeFromJSON(string const &);

  static bool DeserializeFromJSON(string const &, vector<Sample> &);

  string ToStringDebug() const;

private:
  void DeserializeFromJSONImpl(json_t *);

  strings::UniString m_query;
  string m_locale;
  m2::PointD m_pos = m2::PointD(0, 0);
  m2::RectD m_viewport = m2::RectD(0, 0, 0, 0);
  vector<Result> m_results;
};

string DebugPrint(Sample::Result::Relevance);

string DebugPrint(Sample::Result const &);

string DebugPrint(Sample const &);
}  // namespace search
