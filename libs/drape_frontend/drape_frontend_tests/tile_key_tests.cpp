#include "testing/testing.hpp"

#include "drape_frontend/tile_key.hpp"

#include <array>
#include <unordered_map>
#include <unordered_set>

namespace tile_key_tests
{
UNIT_TEST(TileKey_UnorderedContainersIgnoreGenerations)
{
  df::TileKey const tile(-1, 2, 5);
  std::array const generations{df::TileKey(tile, 1, 0), df::TileKey(tile, 0, 1), df::TileKey(tile, 7, 11),
                               df::TileKey(tile, uint64_t{1} << 32, uint64_t{1} << 40)};
  std::unordered_set<df::TileKey> awaiting{tile};
  std::unordered_map<df::TileKey, int> bindings{{tile, 0}};
  for (auto const & key : generations)
  {
    TEST_EQUAL(key, tile, ());
    TEST_EQUAL(std::hash<df::TileKey>{}(key), std::hash<df::TileKey>{}(tile), (key));
    TEST(awaiting.contains(key), (key));
    TEST(!awaiting.insert(key).second, (key));
    TEST(bindings.contains(key), (key));
    TEST(!bindings.insert_or_assign(key, 1).second, (key));
  }
  TEST_EQUAL(awaiting.size(), 1, ());
  TEST_EQUAL(bindings.size(), 1, ());
  TEST_EQUAL(bindings.at(tile), 1, ());
  TEST_EQUAL(awaiting.erase(generations.front()), 1, ());
  TEST_EQUAL(bindings.erase(generations.back()), 1, ());
  TEST(awaiting.empty(), ());
  TEST(bindings.empty(), ());
}

UNIT_TEST(TileKey_UnorderedContainersKeepCoordinatesAndWorldCopiesDistinct)
{
  std::array const tiles{df::TileKey(-1, 0, 3), df::TileKey(0, 0, 3),  df::TileKey(-1, 1, 3), df::TileKey(-1, 0, 4),
                         df::TileKey(3, 0, 3),  df::TileKey(-5, 0, 3), df::TileKey()};
  std::unordered_set<df::TileKey> awaiting;
  std::unordered_map<df::TileKey, size_t> bindings;
  for (size_t i = 0; i < tiles.size(); ++i)
  {
    TEST(awaiting.insert(tiles[i]).second, (tiles[i]));
    TEST(bindings.emplace(tiles[i], i).second, (tiles[i]));
  }
  TEST_EQUAL(awaiting.size(), tiles.size(), ());
  TEST_EQUAL(bindings.size(), tiles.size(), ());
  for (size_t i = 0; i < tiles.size(); ++i)
  {
    df::TileKey const key(tiles[i], 3, 9);
    TEST(awaiting.contains(key), (key));
    TEST_EQUAL(bindings.at(key), i, (key));
    TEST_EQUAL(awaiting.erase(key), 1, (key));
    TEST_EQUAL(bindings.erase(key), 1, (key));
  }
  TEST(awaiting.empty(), ());
  TEST(bindings.empty(), ());
}

UNIT_TEST(TileKey_BatcherIdentityIncludesGenerationsAndBucket)
{
  df::TileKey const tile(-1, 0, 3);
  df::TileKey const geometry(tile, 1, 0);
  df::TileKey const userMarks(tile, 0, 1);
  std::unordered_set<uint64_t> batcherIds;
  for (auto const bucket : {df::BatcherBucket::Default, df::BatcherBucket::Overlay, df::BatcherBucket::UserMark,
                            df::BatcherBucket::Routing, df::BatcherBucket::Traffic, df::BatcherBucket::Transit})
  {
    TEST(batcherIds.insert(tile.GetHashValue(bucket)).second, ());
    TEST(batcherIds.insert(geometry.GetHashValue(bucket)).second, ());
    TEST(batcherIds.insert(userMarks.GetHashValue(bucket)).second, ());
    TEST_EQUAL(tile.GetHashValue(bucket), df::TileKey(3, 0, 3).GetHashValue(bucket), ());
  }
}
}  // namespace tile_key_tests
