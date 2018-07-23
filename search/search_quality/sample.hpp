#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

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
      Irrelevant,
      Relevant,
      Vital
    };

    static Result Build(FeatureType & ft, Relevance relevance);

    bool operator<(Result const & rhs) const;

    bool operator==(Result const & rhs) const;

    m2::PointD m_pos = m2::PointD(0, 0);

    strings::UniString m_name;
    std::string m_houseNumber;
    std::vector<std::string> m_types;  // MAPS.ME types, not OSM types.
    Relevance m_relevance = Relevance::Irrelevant;
  };

  bool DeserializeFromJSON(std::string const & jsonStr);
  my::JSONPtr SerializeToJSON() const;

  static bool DeserializeFromJSONLines(std::string const & lines, std::vector<Sample> & samples);
  static void SerializeToJSONLines(std::vector<Sample> const & samples, std::string & lines);

  bool operator<(Sample const & rhs) const;

  bool operator==(Sample const & rhs) const;

  void DeserializeFromJSONImpl(json_t * root);
  void SerializeToJSONImpl(json_t & root) const;

  void FillSearchParams(search::SearchParams & params) const;

  strings::UniString m_query;
  std::string m_locale;
  m2::PointD m_pos = m2::PointD(0, 0);
  bool m_posAvailable = false;
  m2::RectD m_viewport = m2::RectD(0, 0, 0, 0);
  std::vector<Result> m_results;
  std::vector<strings::UniString> m_relatedQueries;
};

void FromJSONObject(json_t * root, std::string const & field,
                    Sample::Result::Relevance & relevance);
void ToJSONObject(json_t & root, std::string const & field, Sample::Result::Relevance relevance);

void FromJSON(json_t * root, Sample::Result & result);
my::JSONPtr ToJSON(Sample::Result const & result);

std::string DebugPrint(Sample::Result::Relevance r);

std::string DebugPrint(Sample::Result const & r);

std::string DebugPrint(Sample const & s);
}  // namespace search
