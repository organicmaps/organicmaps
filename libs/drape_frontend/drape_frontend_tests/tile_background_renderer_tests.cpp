#include "testing/testing.hpp"

#include "drape_frontend/tile_background_renderer.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/drape_global.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/rect2d.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace tile_background_renderer_tests
{
using df::CoverageResult;
using df::TileBackgroundRenderer;
using df::TileKey;

// A level handover spans an asynchronous read -> decode -> upload -> bind round trip. These tests
// drive the tile lifecycle and verify that retained imagery covers that interval.
class TestTexturePool : public dp::TexturePool
{
public:
  explicit TestTexturePool(uint32_t tilePx = 256)
    : dp::TexturePool(dp::TexturePoolDesc{
          .m_textureWidth = tilePx, .m_textureHeight = tilePx, .m_format = dp::TextureFormat::RGBA8})
  {}

  size_t GetAvailableCount() const override { return 1024; }
  TextureId AcquireTexture(ref_ptr<dp::GraphicsContext>) override { return ++m_lastId; }
  void ReleaseTexture(ref_ptr<dp::GraphicsContext>, TextureId id) override { m_released.push_back(id); }
  void UpdateTextureData(ref_ptr<dp::GraphicsContext>, TextureId, uint32_t, uint32_t, uint32_t, uint32_t,
                         ref_ptr<void>) override
  {}
  ref_ptr<dp::Texture> GetTexture(TextureId) override { return nullptr; }

  std::vector<TextureId> m_released;
  TextureId m_lastId = 0;
};

// The three coverages span the same ground: 2x2 tiles at z16 == 4x4 at z17 == 8x8 at z18.
CoverageResult constexpr kCoverageZ16{.m_minTileX = 0, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 2};
CoverageResult constexpr kCoverageZ17{.m_minTileX = 0, .m_maxTileX = 4, .m_minTileY = 0, .m_maxTileY = 4};
CoverageResult constexpr kCoverageZ18{.m_minTileX = 0, .m_maxTileX = 8, .m_minTileY = 0, .m_maxTileY = 8};

class TileBackgroundRendererTest
{
public:
  TileBackgroundRendererTest()
    : m_renderer([this](TileKey const & key, dp::BackgroundMode)
  {
    m_reads.push_back(key);
    return true;
  }, [this](TileKey const & key, dp::BackgroundMode) { m_cancels.push_back(key); }, dp::BackgroundMode::Satellite)
  {}

  ref_ptr<dp::GraphicsContext> GetContext() { return make_ref<dp::GraphicsContext>(&m_contextImpl); }

  TileKey ResolveRequest(TileKey const & key) const
  {
    if (key.m_generation != 0)
      return key;
    auto const it =
        std::find_if(m_reads.rbegin(), m_reads.rend(), [&key](TileKey const & requested) { return requested == key; });
    return it == m_reads.rend() ? key : *it;
  }

  void UpdateViewport(CoverageResult const & coverage, int zoomLevel)
  {
    m_renderer.OnUpdateViewport(GetContext(), coverage, zoomLevel);
  }

  // Delivers a tile exactly the way the provider does: upload the image, then bind it to the tile.
  void DeliverTile(TileKey const & key)
  {
    AssignTileImage(key);
    BindTileImage(key);
  }

  void AssignTileImage(TileKey const & key)
  {
    auto const requestKey = ResolveRequest(key);
    m_renderer.AssignTileBackgroundImage(GetContext(), requestKey.Coord2String(), make_ref<dp::TexturePool>(&m_pool),
                                         m_pool.AcquireTexture(GetContext()), dp::BackgroundMode::Satellite);
  }

  void BindTileImage(TileKey const & key)
  {
    auto const requestKey = ResolveRequest(key);
    m_renderer.SetTileBackgroundData(GetContext(), requestKey, requestKey.Coord2String(),
                                     m2::RectF(0.0f, 0.0f, 1.0f, 1.0f));
  }

  // Reports a failed read the way the provider does: no image, an empty uid.
  void DeliverFailure(TileKey const & key)
  {
    m_renderer.SetTileBackgroundData(GetContext(), ResolveRequest(key), std::string(),
                                     m2::RectF(0.0f, 0.0f, 1.0f, 1.0f));
  }

  // OnUpdateViewport never re-requests a tile that is already bound, so re-entering a viewport and
  // watching the read callback reports exactly which of its tiles are no longer bound -- i.e. which
  // ones Render() would have no imagery for.
  std::vector<TileKey> ProbeUnboundTiles(CoverageResult const & coverage, int zoomLevel)
  {
    m_reads.clear();
    UpdateViewport(coverage, zoomLevel);
    return m_reads;
  }

  std::vector<TileKey> LoadZ16()
  {
    UpdateViewport(kCoverageZ16, 16);
    TEST_EQUAL(m_reads.size(), size_t{4}, ());
    auto const tiles = m_reads;
    for (auto const & key : tiles)
      DeliverTile(key);
    return tiles;
  }

  std::vector<TileKey> m_reads, m_cancels;
  TestingGraphicsContext m_contextImpl;
  TestTexturePool m_pool;
  TileBackgroundRenderer m_renderer;
};

// The probe itself: a bound tile is never re-requested. Everything below depends on this.
UNIT_CLASS_TEST(TileBackgroundRendererTest, BoundTilesAreNotReRequested)
{
  LoadZ16();
  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("Re-entering the same viewport must request nothing"));
}

// The level being left keeps covering the viewport until the replacement level resolves completely.
UNIT_CLASS_TEST(TileBackgroundRendererTest, ZoomChangeKeepsPreviousLevelUntilLoaded)
{
  LoadZ16();

  m_reads.clear();
  UpdateViewport(kCoverageZ17, 17);
  TEST_EQUAL(m_reads.size(), size_t{16}, ("The new level must still be requested"));

  // One z17 tile lands, 15 are still in flight.
  DeliverTile(m_reads.front());

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("The previous level must stay bound while z17 loads"));
}

// ... but only until the new level is complete, otherwise the layer would keep paying for a second
// set of tiles (memory and overdraw) forever.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FallbackIsRetiredOnceNewLevelIsLoaded)
{
  LoadZ16();

  m_reads.clear();
  UpdateViewport(kCoverageZ17, 17);
  auto const z17Tiles = m_reads;
  for (auto const & key : z17Tiles)
    DeliverTile(key);

  TEST_EQUAL(ProbeUnboundTiles(kCoverageZ16, 16).size(), size_t{4},
             ("The fallback must be released once the new level covers the viewport"));
}

// A pinch crosses several levels quickly. Levels that were passed through without ever being drawn
// must not evict the last level that actually was on screen.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FastMultiLevelZoomKeepsLastDrawnLevel)
{
  LoadZ16();

  UpdateViewport(kCoverageZ17, 17);  // nothing delivered: z17 never reaches the screen
  UpdateViewport(kCoverageZ18, 18);

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("A level that never loaded must not evict the drawn one"));
}

// The realistic pinch: the intermediate level is *partly* loaded when the gesture crosses into the
// next one. A level abandoned mid-load covers only a fraction of the viewport, so it must not
// displace the complete level that is still covering the whole screen.
UNIT_CLASS_TEST(TileBackgroundRendererTest, PartlyLoadedLevelDoesNotEvictCompleteOne)
{
  LoadZ16();

  m_reads.clear();
  UpdateViewport(kCoverageZ17, 17);
  DeliverTile(m_reads.front());  // one z17 tile arrives; 15 are still in flight

  UpdateViewport(kCoverageZ18, 18);  // the gesture crosses into z18

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(),
       ("One tile of a half-loaded level must not evict the complete level covering the screen"));
}

// ... but "not complete" must not mean "discard": a level is dropped when it loses the fallback slot,
// so demanding completeness would blank the layer whenever nothing covers the viewport fully.
// Here a pan leaves reads in flight, and the level still covers most of the screen.
UNIT_CLASS_TEST(TileBackgroundRendererTest, LevelWithReadsInFlightIsStillKept)
{
  LoadZ16();

  // Pan: the four loaded tiles stay in view, two new ones are requested and never arrive.
  CoverageResult constexpr widened{.m_minTileX = 0, .m_maxTileX = 3, .m_minTileY = 0, .m_maxTileY = 2};
  m_reads.clear();
  UpdateViewport(widened, 16);
  TEST_EQUAL(m_reads.size(), size_t{2}, ("Only the newly exposed tiles are requested"));

  // Zoom in over the panned viewport while z16 is still incomplete.
  CoverageResult constexpr widenedZ17{.m_minTileX = 0, .m_maxTileX = 6, .m_minTileY = 0, .m_maxTileY = 4};
  UpdateViewport(widenedZ17, 17);

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("A level with reads in flight still covers the screen"));
}

// Same reasoning zooming out: the old level can never cover the larger viewport (its edges are
// ground nothing has ever loaded), yet it is still the best thing to show while z15 loads.
UNIT_CLASS_TEST(TileBackgroundRendererTest, PartialCoverageIsKeptWhenZoomingOut)
{
  LoadZ16();

  // 2x2 tiles at z15 span four times the ground of the loaded z16 set, which covers only its corner.
  CoverageResult constexpr kCoverageZ15{.m_minTileX = 0, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 2};
  UpdateViewport(kCoverageZ15, 15);

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("Partial cover beats no cover while zooming out"));
}

// Zooming out and back in again is the common gesture; it must not re-fetch anything.
UNIT_CLASS_TEST(TileBackgroundRendererTest, ZoomingBackToARetainedLevelNeedsNoReads)
{
  LoadZ16();

  UpdateViewport(kCoverageZ17, 17);
  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("Returning to a retained level must be instant"));
  // And the abandoned z17 reads are cancelled rather than left dangling.
  TEST_EQUAL(m_cancels.size(), size_t{16}, ());
}

// Tiles of the retained level that the viewport has left behind must still be released, so the
// fallback cannot accumulate while panning.
UNIT_CLASS_TEST(TileBackgroundRendererTest, RetainedLevelIsClippedToViewport)
{
  LoadZ16();

  // Zoom onto the single z17 tile (0, 0), which lies strictly inside the z16 tile (0, 0). Strictly:
  // a coverage spanning the whole z16 tile would also retain its three neighbours, since rects that
  // merely share an edge count as intersecting (m2::Rect::IsIntersect) -- the same inclusive test
  // Render() clips with, so such a tile is an off-screen sliver rather than a wrong one.
  CoverageResult constexpr coverageCorner{.m_minTileX = 0, .m_maxTileX = 1, .m_minTileY = 0, .m_maxTileY = 1};
  UpdateViewport(coverageCorner, 17);

  // Only the still-overlapping z16 tile is kept; the three off-screen ones are gone.
  auto const unbound = ProbeUnboundTiles(kCoverageZ16, 16);
  TEST_EQUAL(unbound.size(), size_t{3}, ("Off-viewport tiles of the retained level must be released"));
}

// Rendering skips a fallback tile once the current level fully hides it (partially transparent
// imagery would composite twice; opaque imagery is overdraw). The predicate is per current-grid
// cell: one missing cell keeps the whole fallback tile drawing beneath.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FallbackCoveredOnlyWhenAllCellsBound)
{
  LoadZ16();
  UpdateViewport(kCoverageZ17, 17);

  TileKey const fallback(0, 0, 16);
  DeliverTile(TileKey(0, 0, 17));
  DeliverTile(TileKey(1, 0, 17));
  DeliverTile(TileKey(0, 1, 17));
  TEST(!m_renderer.IsCoveredByCurrentZoom(fallback), ("One of four cells is still missing"));

  DeliverTile(TileKey(1, 1, 17));
  TEST(m_renderer.IsCoveredByCurrentZoom(fallback), ("All four cells are bound"));
  TEST(!m_renderer.IsCoveredByCurrentZoom(TileKey(1, 0, 16)), ("Sibling fallback tiles stay visible"));
  // A current-level tile is never subject to the fallback occlusion check.
  TEST(!m_renderer.IsCoveredByCurrentZoom(TileKey(0, 0, 17)), ());
}

// Zooming out, the finer fallback tile is hidden by the single current-level ancestor above it.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FinerFallbackCoveredByBoundAncestor)
{
  CoverageResult constexpr covZ17{.m_minTileX = 0, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 2};
  UpdateViewport(covZ17, 17);
  for (auto const & key : std::vector<TileKey>(m_reads))
    DeliverTile(key);

  CoverageResult constexpr covZ16{.m_minTileX = 0, .m_maxTileX = 1, .m_minTileY = 0, .m_maxTileY = 1};
  UpdateViewport(covZ16, 16);

  TileKey const finer(0, 0, 17);
  TEST(!m_renderer.IsCoveredByCurrentZoom(finer), ("The z16 ancestor is not bound yet"));
  DeliverTile(TileKey(0, 0, 16));
  TEST(m_renderer.IsCoveredByCurrentZoom(finer), ("The bound ancestor hides the finer tile"));
}

// Only cells inside the coverage are required: cells beyond it lie outside the inflated viewport
// and cannot be seen. A fallback tile entirely outside the coverage is invisible and trivially
// hidden; a visible one still needs every visible cell.
UNIT_CLASS_TEST(TileBackgroundRendererTest, CellsOutsideCoverageNotRequired)
{
  LoadZ16();

  // Zoom onto the ground of z16 (0, 0) only: its four z17 cells are the whole coverage.
  CoverageResult constexpr cornerZ17{.m_minTileX = 0, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 2};
  UpdateViewport(cornerZ17, 17);

  TEST(m_renderer.IsCoveredByCurrentZoom(TileKey(1, 0, 16)), ("No visible cell -> trivially hidden"));
  TEST(!m_renderer.IsCoveredByCurrentZoom(TileKey(0, 0, 16)), ("All four visible cells are unbound"));
}

// The ancestor lookup must use floor division: round-toward-zero would map the negative-coordinate
// children onto tile (0, 0) instead of (-1, -1).
UNIT_CLASS_TEST(TileBackgroundRendererTest, AncestorLookupUsesFloorForNegativeCoords)
{
  CoverageResult constexpr covZ16{.m_minTileX = -1, .m_maxTileX = 0, .m_minTileY = -1, .m_maxTileY = 0};
  UpdateViewport(covZ16, 16);
  DeliverTile(TileKey(-1, -1, 16));

  TEST(m_renderer.IsCoveredByCurrentZoom(TileKey(-1, -1, 17)), ("floor(-1 / 2) == -1"));
  TEST(m_renderer.IsCoveredByCurrentZoom(TileKey(-2, -2, 17)), ("floor(-2 / 2) == -1"));
  TEST(!m_renderer.IsCoveredByCurrentZoom(TileKey(0, 0, 17)), ("Ancestor (0, 0) is not bound"));
}

// A terminal read result (delivered with an empty uid) resolves its request without binding a tile.
// Once every tile is bound or terminal, the fallback retires.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FailureRetiresFallback)
{
  LoadZ16();

  m_reads.clear();
  UpdateViewport(kCoverageZ17, 17);
  auto const z17Tiles = m_reads;
  TEST_EQUAL(z17Tiles.size(), size_t{16}, ());
  for (size_t i = 0; i + 1 < z17Tiles.size(); ++i)
    DeliverTile(z17Tiles[i]);
  DeliverFailure(z17Tiles.back());

  TEST_EQUAL(ProbeUnboundTiles(kCoverageZ16, 16).size(), size_t{4},
             ("The failure answers the last awaited tile, so the fallback must be released"));
}

// A terminally failed tile is neither bound nor awaited, so a later viewport update can request it
// again if the endpoint's state or credentials have changed.
UNIT_CLASS_TEST(TileBackgroundRendererTest, FailedTileIsRetriedOnNextViewportUpdate)
{
  CoverageResult constexpr coverage{.m_minTileX = 0, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 2};
  UpdateViewport(coverage, 17);
  auto const requested = m_reads;
  TEST_EQUAL(requested.size(), size_t{4}, ());
  for (size_t i = 1; i < requested.size(); ++i)
    DeliverTile(requested[i]);
  DeliverFailure(requested.front());

  m_reads.clear();
  UpdateViewport(coverage, 17);
  TEST_EQUAL(m_reads.size(), size_t{1}, ("Only the failed tile is re-requested"));
  TEST(m_reads.front() == requested.front(), ());
}

// A late failure for a tile that was never requested (or was swept meanwhile) must change nothing.
UNIT_CLASS_TEST(TileBackgroundRendererTest, LateFailureForUnknownTileIsIgnored)
{
  LoadZ16();

  DeliverFailure(TileKey(9, 9, 17));

  TEST(ProbeUnboundTiles(kCoverageZ16, 16).empty(), ("Bindings must be unaffected"));
}

// A provider request can outlive its cancellation. If the tile is requested again before the old
// request completes, the old completion must not consume the new request's awaiting state. Without
// a per-request generation, the following viewport update starts a third, duplicate request while
// the second one is still in flight.
UNIT_CLASS_TEST(TileBackgroundRendererTest, StaleCompletionDoesNotConsumeReRequest)
{
  CoverageResult constexpr first{.m_minTileX = 0, .m_maxTileX = 1, .m_minTileY = 0, .m_maxTileY = 1};
  CoverageResult constexpr second{.m_minTileX = 1, .m_maxTileX = 2, .m_minTileY = 0, .m_maxTileY = 1};
  UpdateViewport(first, 17);  // Request A for |key|.
  TileKey const requestA = m_reads.back();
  UpdateViewport(second, 17);  // Cancel A.
  UpdateViewport(first, 17);   // Request B for the same key.
  TEST_EQUAL(m_cancels.size(), size_t{2}, ());

  DeliverFailure(requestA);  // Late completion of A, while B is still in flight.

  m_reads.clear();
  UpdateViewport(first, 17);
  TEST(m_reads.empty(), ("The current request must remain awaited after an older request completes"));
}

// Image upload and tile binding are separate renderer messages. Completions from multiple worker
// threads may therefore enqueue all uploads before any binding. Pending images must not be treated
// as ordinary unreferenced cache entries and evicted before their matching bind messages arrive.
UNIT_CLASS_TEST(TileBackgroundRendererTest, InterleavedUploadsRemainAvailableForPendingBindings)
{
  CoverageResult constexpr coverage{.m_minTileX = 0, .m_maxTileX = 5, .m_minTileY = 0, .m_maxTileY = 4};
  UpdateViewport(coverage, 17);
  auto const requested = m_reads;
  TEST_EQUAL(requested.size(), size_t{20}, ());

  // The unreferenced image LRU holds 16 entries. Assigning all 20 first deterministically exercises
  // the same ordering as concurrently completed requests whose upload messages overtake bindings.
  for (auto const & key : requested)
    AssignTileImage(key);
  for (auto it = requested.rbegin(); it != requested.rend(); ++it)
    BindTileImage(*it);

  TEST(ProbeUnboundTiles(coverage, 17).empty(), ("Every accepted request must retain its image until binding"));
}
}  // namespace tile_background_renderer_tests
