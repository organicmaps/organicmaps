#include "testing/testing.hpp"

#include "drape_frontend/message_queue.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/tile_background_renderer.hpp"
#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "drape/drape_global.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"
#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/rect2d.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace tile_background_renderer_tests
{
using df::CoverageResult;
using df::TileKey;
using TextureId = dp::TexturePool::TextureId;

class TestTexturePool : public dp::TexturePool
{
public:
  TestTexturePool()
    : dp::TexturePool({.m_textureWidth = 256, .m_textureHeight = 256, .m_format = dp::TextureFormat::RGBA8})
  {}

  size_t GetAvailableCount() const override { return 1024 - m_live.size(); }

  TextureId AcquireTexture(ref_ptr<dp::GraphicsContext>) override
  {
    auto const id = ++m_lastId;
    m_live.insert(id);
    return id;
  }

  void ReleaseTexture(ref_ptr<dp::GraphicsContext>, TextureId id) override
  {
    TEST_EQUAL(m_live.erase(id), size_t{1}, ("A texture must be released exactly once", id));
    m_released.push_back(id);
  }

  void UpdateTextureData(ref_ptr<dp::GraphicsContext>, TextureId, uint32_t, uint32_t, uint32_t, uint32_t,
                         ref_ptr<void>) override
  {}

  ref_ptr<dp::Texture> GetTexture(TextureId) override { return nullptr; }

  std::unordered_set<TextureId> m_live;
  std::vector<TextureId> m_released;
  TextureId m_lastId = 0;
};

CoverageResult Row(int minX, int maxX)
{
  return {.m_minTileX = minX, .m_maxTileX = maxX, .m_minTileY = 0, .m_maxTileY = 1};
}

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

  ~TileBackgroundRendererTest()
  {
    m_renderer.ClearContextDependentResources(GetContext());
    TEST(m_pool.m_live.empty(), ("All owned textures must be released on reset"));
  }

  ref_ptr<dp::GraphicsContext> GetContext() { return make_ref<dp::GraphicsContext>(&m_context); }

  void UpdateViewport(CoverageResult const & coverage, int zoom = 17)
  {
    m_renderer.OnUpdateViewport(GetContext(), coverage, zoom);
  }

  TextureId Deliver(TileKey const & key, std::string const & uid = {}, m2::RectF const & rect = {0, 0, 1, 1},
                    dp::BackgroundMode mode = dp::BackgroundMode::Satellite)
  {
    auto const id = m_pool.AcquireTexture(GetContext());
    m_renderer.AssignTileBackgroundImage(GetContext(), key, uid.empty() ? key.Coord2String() : uid,
                                         make_ref<dp::TexturePool>(&m_pool), id, mode, rect);
    return id;
  }

  void CheckNoNewReads(CoverageResult const & coverage, int zoom = 17)
  {
    m_reads.clear();
    UpdateViewport(coverage, zoom);
    TEST(m_reads.empty(), ("Bound or still awaited tiles must not be requested again", m_reads));
  }

  TestingGraphicsContext m_context;
  TestTexturePool m_pool;
  std::vector<TileKey> m_reads;
  std::vector<TileKey> m_cancels;
  df::TileBackgroundRenderer m_renderer;
};

UNIT_CLASS_TEST(TileBackgroundRendererTest, BurstDeliveriesPinEveryVisibleImage)
{
  auto const coverage = Row(0, 40);
  UpdateViewport(coverage);
  auto const requested = m_reads;
  TEST_EQUAL(requested.size(), size_t{40}, ());

  // A completed image and its binding travel together, even when many completions queue at once.
  for (auto const & key : requested)
  {
    auto const id = Deliver(key);
    TEST(m_pool.m_live.contains(id), (key));
  }

  TEST_EQUAL(m_pool.m_live.size(), requested.size(), ("Visible images must not consume the 16-image unused cache"));
  TEST(m_pool.m_released.empty(), ("No visible image may be evicted during the burst"));
  CheckNoNewReads(coverage);
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, SharedImageSurvivesSiblingCancellation)
{
  UpdateViewport(Row(0, 2));
  auto const shared = Deliver(TileKey(0, 0, 17), "ancestor", {0, 0, 0.5f, 1});
  auto const duplicate = Deliver(TileKey(1, 0, 17), "ancestor", {0.5f, 0, 1, 1});
  auto const rebound = Deliver(TileKey(1, 0, 17), "ancestor", {0.75f, 0, 1, 0.5f});
  TEST(m_pool.m_live.contains(shared), ());
  TEST(!m_pool.m_live.contains(duplicate), ("Sibling uploads must reuse the original image"));
  TEST(!m_pool.m_live.contains(rebound), ("Updating a binding must also deduplicate its upload"));
  CheckNoNewReads(Row(0, 2));

  UpdateViewport(Row(1, 2));
  for (int x = 10; x < 40; ++x)
    Deliver(TileKey(x, 0, 17));

  TEST(m_pool.m_live.contains(shared), ("One remaining sibling must keep the shared texture alive"));
  CheckNoNewReads(Row(1, 2));
  TEST_EQUAL(m_pool.m_live.size(), size_t{17}, ("One shared image and 16 unused images remain"));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, RebindingPinsCachedTargetBeforeReleasingOldImage)
{
  TileKey const key(0, 0, 17);
  UpdateViewport(Row(0, 1));
  auto const previous = Deliver(key, "previous");
  auto const target = Deliver(TileKey(10, 0, 17), "target");
  for (int x = 11; x < 26; ++x)
    Deliver(TileKey(x, 0, 17));
  TEST_EQUAL(m_pool.m_live.size(), size_t{17}, ());

  // The target is the oldest of 16 unused images. Releasing the old binding first would evict it.
  auto const duplicate = Deliver(key, "target");
  TEST(!m_pool.m_live.contains(duplicate), ());
  TEST(m_pool.m_live.contains(target), ("The cached replacement must be pinned before releasing its predecessor"));
  TEST(m_pool.m_live.contains(previous), ("The predecessor enters the unused cache"));
  CheckNoNewReads(Row(0, 1));

  for (int x = 30; x < 50; ++x)
    Deliver(TileKey(x, 0, 17));
  TEST(m_pool.m_live.contains(target), ("The replacement must remain bound under cache pressure"));
  TEST(!m_pool.m_live.contains(previous), ("Its unused predecessor can be evicted"));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, LateCompletionDoesNotRestoreCancelledTile)
{
  TileKey const cancelled(0, 0, 17);
  TileKey const current(1, 0, 17);
  UpdateViewport(Row(0, 1));
  UpdateViewport(Row(1, 2));
  TEST_EQUAL(m_cancels, std::vector<TileKey>{cancelled}, ());

  auto const cached = Deliver(cancelled);
  CheckNoNewReads(Row(1, 2));
  Deliver(current);
  CheckNoNewReads(Row(1, 2));

  m_reads.clear();
  UpdateViewport(Row(0, 1));
  TEST_EQUAL(m_reads, std::vector<TileKey>{cancelled}, ("The cancelled tile must still need a binding"));
  auto const duplicate = Deliver(cancelled);
  TEST(m_pool.m_live.contains(cached), ("A late image can still be reused from the unused cache"));
  TEST(!m_pool.m_live.contains(duplicate), ());
  CheckNoNewReads(Row(0, 1));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, LateZoomCompletionDoesNotAnswerCurrentRead)
{
  UpdateViewport(Row(0, 1), 16);
  UpdateViewport(Row(0, 1), 17);
  TEST_EQUAL(m_cancels, std::vector<TileKey>{TileKey(0, 0, 16)}, ());

  Deliver(TileKey(0, 0, 16));
  CheckNoNewReads(Row(0, 1));
  Deliver(TileKey(0, 0, 17));
  CheckNoNewReads(Row(0, 1));

  m_reads.clear();
  UpdateViewport(Row(0, 1), 16);
  TEST_EQUAL(m_reads, std::vector<TileKey>{TileKey(0, 0, 16)}, ("The stale zoom must not have been rebound"));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, ModeChangeDiscardsLateUploads)
{
  TileKey const key(0, 0, 17);
  UpdateViewport(Row(0, 2));
  Deliver(key, "same-image");
  m_renderer.SetBackgroundMode(GetContext(), dp::BackgroundMode::Default);
  TEST(m_pool.m_live.empty(), ());
  TEST_EQUAL(m_cancels, std::vector<TileKey>{TileKey(1, 0, 17)}, ());

  auto const stale = Deliver(key, "same-image");
  TEST(!m_pool.m_live.contains(stale), ("Images from an inactive mode must be released immediately"));
  CheckNoNewReads(Row(0, 2));

  m_renderer.SetBackgroundMode(GetContext(), dp::BackgroundMode::Satellite);
  TEST_EQUAL(m_reads.size(), size_t{2}, ("Re-enabling the layer must start fresh reads"));
  auto const fresh = Deliver(key, "same-image");
  TEST(m_pool.m_live.contains(fresh), ("The old image must not survive a mode change"));
  CheckNoNewReads(Row(0, 2));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, ResetReleasesBoundCachedAndQueuedTextures)
{
  UpdateViewport(Row(0, 20));
  Deliver(TileKey(0, 0, 17));
  Deliver(TileKey(20, 0, 17));

  df::MessageQueue queue;
  auto const queued = m_pool.AcquireTexture(GetContext());
  queue.PushMessage(make_unique_dp<df::AssignTileBackgroundImageMessage>(
                        GetContext(), TileKey(1, 0, 17), "queued", make_ref<dp::TexturePool>(&m_pool), queued,
                        dp::BackgroundMode::Satellite, m2::RectF(0, 0, 1, 1)),
                    df::MessagePriority::Normal);

  m_renderer.ClearContextDependentResources(GetContext());
  TEST_EQUAL(m_cancels.size(), size_t{19}, ("Only unresolved reads need cancellation"));
  TEST_EQUAL(m_pool.m_live.size(), size_t{1}, ("The undelivered message still owns its texture"));
  queue.InstantFilter([](ref_ptr<df::Message> message) { return message->IsGraphicsContextDependent(); });
  TEST(m_pool.m_live.empty(), ("Context reset must also discard queued uploads"));

  m_renderer.ClearContextDependentResources(GetContext());
  TEST_EQUAL(m_pool.m_released.size(), size_t{3}, ("Repeated cleanup must not release textures twice"));
  m_reads.clear();
  UpdateViewport(Row(0, 20));
  TEST_EQUAL(m_reads.size(), size_t{20}, ("Every tile must be requested again after reset"));
}

UNIT_CLASS_TEST(TileBackgroundRendererTest, PanningEvictsOnlyUnusedImages)
{
  TextureId first = 0;
  TextureId last = 0;
  for (int x = 0; x < 64; ++x)
  {
    UpdateViewport(Row(x, x + 2));
    last = Deliver(TileKey(x, 0, 17));
    if (x == 0)
      first = last;
  }

  TEST_EQUAL(m_cancels.size(), size_t{0}, ("The awaited neighbour stays in coverage until it is delivered"));
  TEST_EQUAL(m_pool.m_live.size(), size_t{17}, ("One bound image and 16 unused images remain"));
  TEST(!m_pool.m_live.contains(first), ("The oldest unused image must be evicted"));
  TEST(m_pool.m_live.contains(last), ("The current image must stay bound"));
  TEST_EQUAL(m_pool.m_released.size(), size_t{47}, ());
  CheckNoNewReads(Row(63, 65));
}
}  // namespace tile_background_renderer_tests
