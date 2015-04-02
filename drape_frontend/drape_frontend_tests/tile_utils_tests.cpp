#include "../../testing/testing.hpp"

#include "../../drape_frontend/tile_utils.hpp"

namespace
{

UNIT_TEST(TileUtils_EqualityTest)
{
  df::TileKey key(1 /* x */, 2 /* y */, 2 /* zoom level */);
  set<df::TileKey> output;
  df::CalcTilesCoverage(key, 2 /* targetZoom */, output);

  TEST_EQUAL(output.size(), 1, ());
  TEST_EQUAL(*output.begin(), key, ());
}

UNIT_TEST(TileUtils_MinificationTest)
{
  df::TileKey key(500 /* x */, 300 /* y */, 12 /* zoom level */);
  set<df::TileKey> output;
  df::CalcTilesCoverage(key, 11 /* targetZoom */, output);
  TEST_EQUAL(output.size(), 1, ());
  TEST_EQUAL(*output.begin(), df::TileKey(250, 150, 11), ());

  df::TileKey key2(-351 /* x */, 300 /* y */, 12 /* zoom level */);
  set<df::TileKey> output2;
  df::CalcTilesCoverage(key2, 10 /* targetZoom */, output2);
  TEST_EQUAL(output2.size(), 1, ());
  TEST_EQUAL(*output2.begin(), df::TileKey(-88, 75, 10), ());

  set<df::TileKey> tileKeys =
  {
    df::TileKey(-351 /* x */, 300 /* y */, 12 /* zoom level */),
    df::TileKey(-350 /* x */, 300 /* y */, 12 /* zoom level */)
  };
  set<df::TileKey> output3;
  df::CalcTilesCoverage(tileKeys, 10 /* targetZoom */, output3);
  TEST_EQUAL(output3.size(), 1, ());
  TEST_EQUAL(*output3.begin(), df::TileKey(-88, 75, 10), ());

  TEST_EQUAL(df::IsTileAbove(key, df::TileKey(250, 150, 11)), true, ());
}

UNIT_TEST(TileUtils_MagnificationTest)
{
  df::TileKey key(1 /* x */, 2 /* y */, 2 /* zoom level */);
  set<df::TileKey> output;
  df::CalcTilesCoverage(key, 4 /* targetZoom */, output);

  set<df::TileKey> expectedResult =
  {
    df::TileKey(4, 8, 4), df::TileKey(5, 8, 4), df::TileKey(6, 8, 4), df::TileKey(7, 8, 4),
    df::TileKey(4, 9, 4), df::TileKey(5, 9, 4), df::TileKey(6, 9, 4), df::TileKey(7, 9, 4),
    df::TileKey(4, 10, 4), df::TileKey(5, 10, 4), df::TileKey(6, 10, 4), df::TileKey(7, 10, 4),
    df::TileKey(4, 11, 4), df::TileKey(5, 11, 4), df::TileKey(6, 11, 4), df::TileKey(7, 11, 4)
  };

  TEST_EQUAL(output, expectedResult, ());

  df::TileKey key2(-1 /* x */, -2 /* y */, 2 /* zoom level */);
  set<df::TileKey> output2;
  df::CalcTilesCoverage(key2, 4 /* targetZoom */, output2);

  set<df::TileKey> expectedResult2 =
  {
    df::TileKey(-4, -8, 4), df::TileKey(-3, -8, 4), df::TileKey(-2, -8, 4), df::TileKey(-1, -8, 4),
    df::TileKey(-4, -7, 4), df::TileKey(-3, -7, 4), df::TileKey(-2, -7, 4), df::TileKey(-1, -7, 4),
    df::TileKey(-4, -6, 4), df::TileKey(-3, -6, 4), df::TileKey(-2, -6, 4), df::TileKey(-1, -6, 4),
    df::TileKey(-4, -5, 4), df::TileKey(-3, -5, 4), df::TileKey(-2, -5, 4), df::TileKey(-1, -5, 4)
  };

  TEST_EQUAL(output2, expectedResult2, ());

  for (df::TileKey const & k : expectedResult)
    TEST_EQUAL(df::IsTileBelow(key, k), true, ());
}

}
