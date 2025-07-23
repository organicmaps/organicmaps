#include "testing/testing.hpp"

#include "search/search_quality/sample.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <vector>

namespace sample_test
{
using search::Sample;

class SampleTest
{
public:
  SampleTest() { Init(); }

protected:
  void Init();

  Sample m_cuba;
  Sample m_riga;
  Sample m_tula;
};

void SampleTest::Init()
{
  m_cuba.m_query = strings::MakeUniString("cuba");
  m_cuba.m_locale = "en";
  m_cuba.m_pos = m2::PointD{37.618706, 99.53730574302003};
  m_cuba.m_viewport = {37.1336, 67.1349, 38.0314, 67.7348};
  Sample::Result cubaRes;
  cubaRes.m_name = strings::MakeUniString("Cuba");
  cubaRes.m_relevance = Sample::Result::Relevance::Relevant;
  cubaRes.m_types.push_back("place-country");
  cubaRes.m_pos = {-80.832886, 15.521132748163712};
  cubaRes.m_houseNumber = "";
  m_cuba.m_results = {cubaRes};
  m_cuba.m_relatedQueries = {strings::MakeUniString("Cuba Libre"), strings::MakeUniString("Patria o Muerte")};

  m_riga.m_query = strings::MakeUniString("riga");
  m_riga.m_locale = "en";
  m_riga.m_pos = m2::PointD{37.65376, 98.51110651930014};
  m_riga.m_viewport = {37.5064, 67.0476, 37.7799, 67.304};
  Sample::Result rigaRes;
  rigaRes.m_name = strings::MakeUniString("Rīga");
  rigaRes.m_relevance = Sample::Result::Relevance::Vital;
  rigaRes.m_types.push_back("place-city-capital-2");
  rigaRes.m_pos = {24.105186, 107.7819569220319};
  rigaRes.m_houseNumber = "";
  m_riga.m_results = {rigaRes, rigaRes};

  m_tula.m_query = strings::MakeUniString("tula");
  m_tula.m_locale = "en";
  m_tula.m_pos = {};
  m_tula.m_viewport = {37.5064, 67.0476, 37.7799, 67.304};
}

UNIT_CLASS_TEST(SampleTest, Smoke)
{
  auto const jsonStr = R"EOF(
  {
    "query": "cuba",
    "locale": "en",
    "position": {
      "x": 37.618706,
      "y": 99.53730574302003
    },
    "viewport": {
      "minx": 37.1336,
      "miny": 67.1349,
      "maxx": 38.0314,
      "maxy": 67.7348
    },
    "results": [
      {
        "name": "Cuba",
        "relevancy": "relevant",
        "types": [
          "place-country"
        ],
        "position": {
          "x": -80.832886,
          "y": 15.521132748163712
        },
        "houseNumber": ""
      }
    ],
    "related_queries": ["Cuba Libre", "Patria o Muerte"]
  }
  )EOF";

  Sample s;
  TEST(s.DeserializeFromJSON(jsonStr), ());
  TEST_EQUAL(s, m_cuba, ());
}

UNIT_CLASS_TEST(SampleTest, BadViewport)
{
  auto const jsonStr = R"EOF(
  {
    "results": [
      {
        "houseNumber": "",
        "position": {
          "y": 15.521132748163712,
          "x": -80.832886
        },
        "types": [
          "place-country"
        ],
        "relevancy": "relevant",
        "name": "Cuba"
      }
    ],
    "viewport": {
      "maxy": 67.7348,
      "maxx": 38.0314,
    },
    "position": {
      "y": 99.53730574302003,
      "x": 37.618706
    },
    "locale": "en",
    "query": "cuba"
  }
  )EOF";

  Sample s;
  TEST(!s.DeserializeFromJSON(jsonStr), ());
}

UNIT_CLASS_TEST(SampleTest, Arrays)
{
  std::string lines;
  lines.append(
      R"({"query": "cuba", "locale": "en", "position": {"x": 37.618706, "y": 99.53730574302003}, "viewport": {"minx": 37.1336, "miny": 67.1349, "maxx": 38.0314, "maxy": 67.7348}, "results": [{"name": "Cuba", "relevancy": "relevant", "types": ["place-country"], "position": {"x": -80.832886, "y": 15.521132748163712}, "houseNumber": ""}], "related_queries": ["Patria o Muerte", "Cuba Libre"]})");
  lines.append("\n");

  lines.append(
      R"({"query": "riga", "locale": "en", "position": {"x": 37.65376, "y": 98.51110651930014}, "viewport": {"minx": 37.5064, "miny": 67.0476, "maxx": 37.7799, "maxy": 67.304}, "results": [{"name": "R\u012bga", "relevancy": "vital", "types": ["place-city-capital-2"], "position": {"x": 24.105186, "y": 107.7819569220319}, "houseNumber": ""}, {"name": "R\u012bga", "relevancy": "vital", "types": ["place-city-capital-2"], "position": {"x": 24.105186, "y": 107.7819569220319}, "houseNumber": ""}]})");
  lines.append("\n");

  lines.append(
      R"({"query": "tula", "locale": "en", "viewport": {"minx": 37.5064, "miny": 67.0476, "maxx": 37.7799, "maxy": 67.304}})");
  lines.append("\n");

  std::vector<Sample> samples;
  TEST(Sample::DeserializeFromJSONLines(lines, samples), ());

  std::vector<Sample> expected = {m_cuba, m_riga, m_tula};

  std::sort(samples.begin(), samples.end());
  std::sort(expected.begin(), expected.end());

  TEST_EQUAL(samples, expected, ());
}

UNIT_CLASS_TEST(SampleTest, SerDes)
{
  std::vector<Sample> expected = {m_cuba, m_riga, m_tula};

  std::string lines;
  Sample::SerializeToJSONLines(expected, lines);

  std::vector<Sample> actual;
  TEST(Sample::DeserializeFromJSONLines(lines, actual), ());

  std::sort(expected.begin(), expected.end());
  std::sort(actual.begin(), actual.end());
  TEST_EQUAL(expected, actual, ());
}
}  // namespace sample_test
