#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "cppjansson/cppjansson.hpp"

#include <optional>
#include <string>
#include <vector>

class FeatureType;

namespace search
{
struct SearchParams;

struct Sample
{
  struct Result
  {
    enum class Relevance
    {
      // A result that should not be present and it's hard (for the user)
      // to explain why it is there, i.e. it is a waste of time even
      // to try to understand what this result is about.
      Harmful,

      // A result that is irrelevant to the query but at
      // least it is easy to explain why it showed up.
      Irrelevant,

      // A result that is relevant to the query.
      Relevant,

      // A result that definetely should be present, preferably
      // at a position close to the beginning.
      Vital
    };

    static Result Build(FeatureType & ft, Relevance relevance);

    bool operator<(Result const & rhs) const;

    bool operator==(Result const & rhs) const;

    m2::PointD m_pos = m2::PointD(0, 0);

    strings::UniString m_name;
    std::string m_houseNumber;
    std::vector<std::string> m_types;  // OMaps types, not OSM types.
    Relevance m_relevance = Relevance::Irrelevant;
  };

  bool DeserializeFromJSON(std::string const & jsonStr);
  base::JSONPtr SerializeToJSON() const;

  static bool DeserializeFromJSONLines(std::string const & lines, std::vector<Sample> & samples);
  static void SerializeToJSONLines(std::vector<Sample> const & samples, std::string & lines);

  bool operator<(Sample const & rhs) const;

  bool operator==(Sample const & rhs) const;

  void DeserializeFromJSONImpl(json_t * root);
  void SerializeToJSONImpl(json_t & root) const;

  void FillSearchParams(search::SearchParams & params) const;

  strings::UniString m_query;
  std::string m_locale;
  std::optional<m2::PointD> m_pos;
  m2::RectD m_viewport = m2::RectD(0, 0, 0, 0);
  std::vector<Result> m_results;
  std::vector<strings::UniString> m_relatedQueries;

  // A useless sample is usually a result of the user exploring
  // the search engine without a clear search intent or a sample
  // that cannot be assessed properly using only the data available
  // to the engine (for example, related queries may help a lot but
  // are not expected to be available, or local knowledge of the area
  // is needed).
  // More examples:
  // * A sample whose requests is precisely about a particular street
  //   in a particular city is useless if the assessor is sure that
  //   there is no such street in this city.
  // * On the other hand, if there is such a street (or, more often,
  //   a building) as indicated by other data sources but the engine
  //   still could not find it because of its absense in our
  //   data, the sample is NOT useless.
  bool m_useless = false;
};

void FromJSONObject(json_t * root, char const * field, Sample::Result::Relevance & relevance);
void ToJSONObject(json_t & root, char const * field, Sample::Result::Relevance relevance);
void FromJSONObject(json_t * root, std::string const & field, Sample::Result::Relevance & relevance);
void ToJSONObject(json_t & root, std::string const & field, Sample::Result::Relevance relevance);

void FromJSON(json_t * root, Sample::Result & result);
base::JSONPtr ToJSON(Sample::Result const & result);

std::string DebugPrint(Sample::Result::Relevance r);

std::string DebugPrint(Sample::Result const & r);

std::string DebugPrint(Sample const & s);
}  // namespace search
