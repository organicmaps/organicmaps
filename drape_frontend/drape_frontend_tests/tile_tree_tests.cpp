#include "testing/testing.hpp"

#include "drape_frontend/tile_tree_builder.hpp"

#include "drape/glstate.hpp"

#include "indexer/mercator.hpp"

#include "base/logging.hpp"

namespace
{

class TileTreeTester
{
public:
  TileTreeTester()
  {
    m_tree = make_unique<df::TileTree>();
  }

  void RequestTiles(int const tileScale, m2::RectD const & clipRect)
  {
    double const range = MercatorBounds::maxX - MercatorBounds::minX;
    double const rectSize = range / (1 << tileScale);
    int const minTileX = static_cast<int>(floor(clipRect.minX() / rectSize));
    int const maxTileX = static_cast<int>(ceil(clipRect.maxX() / rectSize));
    int const minTileY = static_cast<int>(floor(clipRect.minY() / rectSize));
    int const maxTileY = static_cast<int>(ceil(clipRect.maxY() / rectSize));

    m_tree->BeginRequesting(tileScale, clipRect);
    for (int tileY = minTileY; tileY < maxTileY; ++tileY)
    {
      for (int tileX = minTileX; tileX < maxTileX; ++tileX)
      {
        df::TileKey key(tileX, tileY, tileScale);
        if (clipRect.IsIntersect(key.GetGlobalRect()))
          m_tree->RequestTile(key);
      }
    }
    m_tree->EndRequesting();
  }

  void FlushTile(int const currentZoomLevel, df::TileKey const & tileKey)
  {
    m_tree->ProcessTile(tileKey, currentZoomLevel, dp::GLState(0, dp::GLState::DepthLayer::GeometryLayer), nullptr);
  }

  void FinishTiles(int const currentZoomLevel, df::TTilesCollection const & tiles)
  {
    m_tree->FinishTiles(tiles, currentZoomLevel);
  }

  unique_ptr<df::TileTree> const & GetTree()
  {
    return m_tree;
  }

private:
  unique_ptr<df::TileTree> m_tree;
};

UNIT_TEST(TileTree_TilesRequesting)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4), TileStatus::Requested)
                                             .Node(TileKey(0, -1, 4), TileStatus::Requested)
                                             .Node(TileKey(-1, 0, 4), TileStatus::Requested)
                                             .Node(TileKey(0, 0, 4), TileStatus::Requested));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_TilesFlushing)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));
  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(0, 0, 4));

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4), TileStatus::Requested)
                                             .Node(TileKey(0, -1, 4), TileStatus::Rendered)
                                             .Node(TileKey(-1, 0, 4), TileStatus::Requested)
                                             .Node(TileKey(0, 0, 4), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_TilesFinishing)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));
  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(0, 0, 4));
  TTilesCollection tiles = { TileKey(-1, -1, 4), TileKey(-1, 0, 4) };
  treeTester.FinishTiles(4, tiles);

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4), TileStatus::Rendered)
                                             .Node(TileKey(0, -1, 4), TileStatus::Rendered)
                                             .Node(TileKey(-1, 0, 4), TileStatus::Rendered)
                                             .Node(TileKey(0, 0, 4), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_MapShifting)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));
  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(0, 0, 4));
  treeTester.RequestTiles(4, m2::RectD(-10, -20, 30, 20));

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4), TileStatus::Requested)
                                             .Node(TileKey(0, -1, 4), TileStatus::Rendered)
                                             .Node(TileKey(-1, 0, 4), TileStatus::Requested)
                                             .Node(TileKey(0, 0, 4), TileStatus::Rendered)
                                             .Node(TileKey(1, -1, 4), TileStatus::Requested)
                                             .Node(TileKey(1, 0, 4), TileStatus::Requested));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_MapMagnification)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));
  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(0, 0, 4));
  treeTester.FlushTile(4, TileKey(-1, -1, 4));
  treeTester.FlushTile(4, TileKey(-1, 0, 4));
  treeTester.RequestTiles(5, m2::RectD(-20, -20, 20, 20));

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(-2, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-2, -1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, -1, 5), TileStatus::Requested))
                                             .Node(TileKey(0, -1, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(0, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(0, -1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, -1, 5), TileStatus::Requested))
                                             .Node(TileKey(-1, 0, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-2, 1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                                             .Node(TileKey(0, 0, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(0, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(0, 1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, 1, 5), TileStatus::Requested)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(-1, -2, 5));
  treeTester.FlushTile(5, TileKey(-2, -1, 5));
  treeTester.FlushTile(5, TileKey(0, -1, 5));
  treeTester.FlushTile(5, TileKey(-2, 1, 5));
  treeTester.FlushTile(5, TileKey(1, 0, 5));
  treeTester.FlushTile(5, TileKey(0, 1, 5));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Rendered).Children(Node(TileKey(-2, -2, 5), TileStatus::Requested)
                                                           .Node(TileKey(-1, -2, 5), TileStatus::Deferred)
                                                           .Node(TileKey(-2, -1, 5), TileStatus::Deferred)
                                                           .Node(TileKey(-1, -1, 5), TileStatus::Requested))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, -2, 5), TileStatus::Requested)
                                                            .Node(TileKey(1, -2, 5), TileStatus::Requested)
                                                            .Node(TileKey(0, -1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, -1, 5), TileStatus::Requested))
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-1, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-2, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(1, 0, 5), TileStatus::Deferred)
                                                            .Node(TileKey(0, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, 1, 5), TileStatus::Requested)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(-2, -2, 5));
  treeTester.FlushTile(5, TileKey(-1, -1, 5));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Unknown,
                              true /* isRemoved */).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, -2, 5), TileStatus::Requested)
                                                            .Node(TileKey(1, -2, 5), TileStatus::Requested)
                                                            .Node(TileKey(0, -1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, -1, 5), TileStatus::Requested))
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-1, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-2, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(1, 0, 5), TileStatus::Deferred)
                                                            .Node(TileKey(0, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, 1, 5), TileStatus::Requested)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(0, -2, 5));

  // a tile comes several times
  treeTester.FlushTile(5, TileKey(1, -2, 5));
  treeTester.FlushTile(5, TileKey(1, -2, 5));
  treeTester.FlushTile(5, TileKey(1, -2, 5));
  treeTester.FlushTile(5, TileKey(1, -2, 5));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Unknown,
                              true /* isRemoved */).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, -2, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, -2, 5), TileStatus::Deferred)
                                                            .Node(TileKey(0, -1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, -1, 5), TileStatus::Requested))
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-1, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-2, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(0, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(1, 0, 5), TileStatus::Deferred)
                                                            .Node(TileKey(0, 1, 5), TileStatus::Deferred)
                                                            .Node(TileKey(1, 1, 5), TileStatus::Requested)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(1, -1, 5));
  treeTester.FlushTile(5, TileKey(-2, 0, 5));
  treeTester.FlushTile(5, TileKey(-1, 0, 5));
  treeTester.FlushTile(5, TileKey(-1, 1, 5));
  treeTester.FlushTile(5, TileKey(0, 0, 5));
  treeTester.FlushTile(5, TileKey(1, 1, 5));

  result = builder.Build(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(0, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(1, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(0, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(1, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(0, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(1, 1, 5), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_MapMinification)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(5, m2::RectD(-20, -20, 20, 20));
  TTilesCollection tiles = { TileKey(-2, -2, 5), TileKey(-1, -2, 5), TileKey(-2, -1, 5), TileKey(-1, -1, 5),
                             TileKey(0, -2, 5), TileKey(1, -2, 5), TileKey(0, -1, 5), TileKey(1, -1, 5),
                             TileKey(-2, 0, 5), TileKey(-1, 0, 5), TileKey(-2, 1, 5), TileKey(-1, 1, 5),
                             TileKey(0, 0, 5), TileKey(1, 0, 5), TileKey(0, 1, 5), TileKey(1, 1, 5) };
  treeTester.FinishTiles(5, tiles);

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                             .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                             .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                             .Node(TileKey(-1, -1, 5), TileStatus::Rendered)
                                             .Node(TileKey(0, -2, 5), TileStatus::Rendered)
                                             .Node(TileKey(1, -2, 5), TileStatus::Rendered)
                                             .Node(TileKey(0, -1, 5), TileStatus::Rendered)
                                             .Node(TileKey(1, -1, 5), TileStatus::Rendered)
                                             .Node(TileKey(-2, 0, 5), TileStatus::Rendered)
                                             .Node(TileKey(-1, 0, 5), TileStatus::Rendered)
                                             .Node(TileKey(-2, 1, 5), TileStatus::Rendered)
                                             .Node(TileKey(-1, 1, 5), TileStatus::Rendered)
                                             .Node(TileKey(0, 0, 5), TileStatus::Rendered)
                                             .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                                             .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                                             .Node(TileKey(1, 1, 5), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Requested).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Requested).Children(Node(TileKey(0, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(0, -1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Requested).Children(Node(TileKey(-2, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-2, 1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, 1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Requested).Children(Node(TileKey(0, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, 1, 5), TileStatus::Rendered)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(-1, 0, 4));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Requested).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Rendered)
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Rendered)
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Requested).Children(Node(TileKey(0, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                                                             .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                                                             .Node(TileKey(1, 1, 5), TileStatus::Rendered)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(4, TileKey(-1, -1, 4));
  treeTester.FlushTile(4, TileKey(0, 0, 4));

  result = builder.Build(Node(TileKey(-1, -1, 4), TileStatus::Rendered)
                        .Node(TileKey(0, -1, 4), TileStatus::Rendered)
                        .Node(TileKey(-1, 0, 4), TileStatus::Rendered)
                        .Node(TileKey(0, 0, 4), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}

UNIT_TEST(TileTree_MapMagMin)
{
  using namespace df;

  TileTreeBuilder builder;
  TileTreeComparer comparer;
  TileTreeTester treeTester;

  treeTester.RequestTiles(5, m2::RectD(-20, -20, 20, 20));
  TTilesCollection tiles = { TileKey(-2, -2, 5), TileKey(-1, -2, 5), TileKey(-2, -1, 5), TileKey(-1, -1, 5),
                             TileKey(0, -2, 5), TileKey(1, -2, 5), TileKey(0, -1, 5), TileKey(1, -1, 5),
                             TileKey(-2, -0, 5), TileKey(-1, 0, 5), TileKey(-2, 1, 5), TileKey(-1, 1, 5),
                             TileKey(0, 0, 5), TileKey(1, 0, 5), TileKey(0, 1, 5), TileKey(1, 1, 5) };
  treeTester.FinishTiles(5, tiles);
  treeTester.RequestTiles(4, m2::RectD(-20, -20, 20, 20));
  treeTester.FlushTile(4, TileKey(0, -1, 4));
  treeTester.FlushTile(4, TileKey(-1, 0, 4));
  // Zoom in 5th level again
  treeTester.RequestTiles(5, m2::RectD(-20, -20, 20, 20));

  unique_ptr<TileTree> result = builder.Build(Node(TileKey(-1, -1, 4),
                                                   TileStatus::Unknown).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                                             .Node(TileKey(0, -1, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(0, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, -2, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(0, -1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(1, -1, 5), TileStatus::Requested))
                                             .Node(TileKey(-1, 0, 4),
                                                   TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, 0, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-2, 1, 5), TileStatus::Requested)
                                                                                 .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                                             .Node(TileKey(0, 0, 4),
                                                   TileStatus::Unknown).Children(Node(TileKey(0, 0, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                                                                                .Node(TileKey(1, 1, 5), TileStatus::Rendered)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  // current zoom level = 5, but tile (-1, -1, 4) is requested to flush
  treeTester.FlushTile(5, TileKey(-1, -1, 4));
  // the tree must be the same
  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(0, -2, 5));
  treeTester.FlushTile(5, TileKey(1, -2, 5));
  treeTester.FlushTile(5, TileKey(0, -1, 5));
  treeTester.FlushTile(5, TileKey(1, -1, 5));
  treeTester.FlushTile(5, TileKey(-1, 0, 5));

  result = builder.Build(Node(TileKey(-1, -1, 4),
                              TileStatus::Unknown).Children(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                                                           .Node(TileKey(-1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(0, -1, 4),
                              TileStatus::Unknown,
                              true /* isRemoved */).Children(Node(TileKey(0, -2, 5), TileStatus::Rendered)
                                                            .Node(TileKey(1, -2, 5), TileStatus::Rendered)
                                                            .Node(TileKey(0, -1, 5), TileStatus::Rendered)
                                                            .Node(TileKey(1, -1, 5), TileStatus::Rendered))
                        .Node(TileKey(-1, 0, 4),
                              TileStatus::Rendered).Children(Node(TileKey(-2, 0, 5), TileStatus::Requested)
                                                            .Node(TileKey(-1, 0, 5), TileStatus::Deferred)
                                                            .Node(TileKey(-2, 1, 5), TileStatus::Requested)
                                                            .Node(TileKey(-1, 1, 5), TileStatus::Requested))
                        .Node(TileKey(0, 0, 4),
                              TileStatus::Unknown).Children(Node(TileKey(0, 0, 5), TileStatus::Rendered)
                                                           .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                                                           .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                                                           .Node(TileKey(1, 1, 5), TileStatus::Rendered)));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));

  treeTester.FlushTile(5, TileKey(-2, 0, 5));
  treeTester.FlushTile(5, TileKey(-2, 1, 5));
  treeTester.FlushTile(5, TileKey(-1, 1, 5));

  result = builder.Build(Node(TileKey(-2, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(0, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(1, -2, 5), TileStatus::Rendered)
                        .Node(TileKey(0, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(1, -1, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(-2, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(-1, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(0, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(1, 0, 5), TileStatus::Rendered)
                        .Node(TileKey(0, 1, 5), TileStatus::Rendered)
                        .Node(TileKey(1, 1, 5), TileStatus::Rendered));

  TEST(comparer.IsEqual(treeTester.GetTree(), result), ("Tree = ", treeTester.GetTree(), "Result = ", result));
}


}
