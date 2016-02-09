#include "testing/testing.hpp"

#include "search/sample.hpp"

#include "base/logging.hpp"

using namespace search;

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

  LOG(LINFO, (DebugPrint(s)));
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
}
