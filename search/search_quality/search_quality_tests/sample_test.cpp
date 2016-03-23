#include "testing/testing.hpp"

#include "search/search_quality/sample.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

using namespace search;

namespace
{
search::Sample g_cuba;
search::Sample g_riga;

void FillGlobals()
{
  g_cuba.m_query = strings::MakeUniString("cuba");
  g_cuba.m_locale = "en";
  g_cuba.m_pos = {37.618706, 99.53730574302003};
  g_cuba.m_viewport = {37.1336, 67.1349, 38.0314, 67.7348};
  search::Sample::Result cubaRes;
  cubaRes.m_name = strings::MakeUniString("Cuba");
  cubaRes.m_relevance = search::Sample::Result::RELEVANCE_RELEVANT;
  cubaRes.m_types.push_back("place-country");
  cubaRes.m_pos = {-80.832886, 15.521132748163712};
  cubaRes.m_houseNumber = "";
  g_cuba.m_results = {cubaRes};

  g_riga.m_query = strings::MakeUniString("riga");
  g_riga.m_locale = "en";
  g_riga.m_pos = {37.65376, 98.51110651930014};
  g_riga.m_viewport = {37.5064, 67.0476, 37.7799, 67.304};
  search::Sample::Result rigaRes;
  rigaRes.m_name = strings::MakeUniString("Rīga");
  rigaRes.m_relevance = search::Sample::Result::RELEVANCE_VITAL;
  rigaRes.m_types.push_back("place-city-capital-2");
  rigaRes.m_pos = {24.105186, 107.7819569220319};
  rigaRes.m_houseNumber = "";
  g_riga.m_results = {rigaRes, rigaRes};
}
}  // namespace

UNIT_TEST(Sample_Smoke)
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
    ]
  }
  )EOF";

  Sample s;
  TEST(s.DeserializeFromJSON(jsonStr), ());
  FillGlobals();
  TEST_EQUAL(s, g_cuba, ());
}

UNIT_TEST(Sample_BadViewport)
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

UNIT_TEST(Sample_Arrays)
{
  auto const jsonStr = R"EOF(
  [
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
      ]
    },
    {
      "query": "riga",
      "locale": "en",
      "position": {
        "x": 37.65376,
        "y": 98.51110651930014
      },
      "viewport": {
        "minx": 37.5064,
        "miny": 67.0476,
        "maxx": 37.7799,
        "maxy": 67.304
      },
      "results": [
        {
          "name": "R\u012bga",
          "relevancy": "vital",
          "types": [
            "place-city-capital-2"
          ],
          "position": {
            "x": 24.105186,
            "y": 107.7819569220319
          },
          "houseNumber": ""
        },
        {
          "name": "R\u012bga",
          "relevancy": "vital",
          "types": [
            "place-city-capital-2"
          ],
          "position": {
            "x": 24.105186,
            "y": 107.7819569220319
          },
          "houseNumber": ""
        }
      ]
    }
  ]
  )EOF";

  vector<Sample> samples;
  TEST(Sample::DeserializeFromJSON(jsonStr, samples), ());

  FillGlobals();
  vector<Sample> expected = {g_cuba, g_riga};

  sort(samples.begin(), samples.end());
  sort(expected.begin(), expected.end());

  TEST_EQUAL(samples, expected, ());
}
