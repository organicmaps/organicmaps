#include "testing/testing.hpp"

#include "drape_frontend/batchers_pool.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/message_queue.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/render_state_extension.hpp"
#include "drape_frontend/requested_tiles.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/tile_message_filter.hpp"

namespace tile_cancellation_tests
{
df::TileKey MakeReadKey(int x = 0)
{
  df::TileKey key(df::TileKey(x, 0, 10), 1 /* generation */, 1 /* userMarksGeneration */);
  key.InitReadState();
  return key;
}

dp::RenderState MakeRenderState()
{
  return df::CreateRenderState(gpu::Program::Area, df::DepthLayer::GeometryLayer);
}

void FilterCancelled(df::MessageQueue & queue)
{
  queue.InstantFilter([](auto const message)
  { return df::FilterTileMessage(message, [](auto const & key) { return key.IsCancelled(); }); });
}

UNIT_TEST(TileCancellation_PropagatesThroughCompletionAndUserMarks)
{
  auto const key = MakeReadKey();
  auto context = make_unique_dp<df::EngineContext>(key, nullptr, nullptr, nullptr, df::CustomFeaturesContextWeakPtr{},
                                                   false, false, false, 0, dp::BackgroundMode::Default, 1.0f);
  df::TileInfo tile(std::move(context));
  df::TileKey const updatedMarksKey(key, key.m_generation, key.m_userMarksGeneration + 1);
  auto completion = make_unique_dp<df::FinishTileReadMessage>(df::TTilesCollection{updatedMarksKey}, true);

  TEST(!tile.IsCancelled(), ());
  TEST(!updatedMarksKey.IsCancelled(), ());
  tile.Cancel();
  TEST(key.IsCancelled(), ());
  TEST(updatedMarksKey.IsCancelled(), ());
  TEST(completion->GetTiles().begin()->IsCancelled(), ());
  TEST(!df::FilterTileMessage(make_ref(completion), [](auto const & k) { return k.IsCancelled(); }), ());
}

UNIT_TEST(TileCancellation_ReenterKeepsNewQueuedGeometry)
{
  auto const oldKey = MakeReadKey();
  auto const newKey = MakeReadKey();
  TEST(oldKey.EqualStrict(newKey), ());

  df::MessageQueue queue;
  auto const state = MakeRenderState();
  queue.PushMessage(make_unique_dp<df::FlushRenderBucketMessage>(oldKey, state, nullptr), df::MessagePriority::Normal);
  queue.PushMessage(make_unique_dp<df::FlushRenderBucketMessage>(newKey, state, nullptr), df::MessagePriority::Normal);

  oldKey.CancelRead();
  FilterCancelled(queue);

  auto const remaining = queue.PopMessage(false);
  TEST(remaining != nullptr, ());
  TEST(!ref_ptr<df::FlushRenderBucketMessage>(make_ref(remaining))->GetKey().IsCancelled(), ());
  TEST(queue.PopMessage(false) == nullptr, ());

  auto lateMessage = make_unique_dp<df::FlushRenderBucketMessage>(oldKey, state, nullptr);
  TEST(lateMessage->GetKey().IsCancelled(), ());
}

UNIT_TEST(TileCancellation_MixedBatchesKeepLiveReadsAndNotifications)
{
  auto const oldKey = MakeReadKey();
  auto const newKey = MakeReadKey();
  auto const neighborKey = MakeReadKey(1);
  auto const state = MakeRenderState();
  df::MessageQueue queue;

  df::TOverlaysRenderData overlays;
  for (auto const & key : {oldKey, newKey, oldKey, neighborKey})
    overlays.emplace_back(key, state, nullptr);
  queue.PushMessage(make_unique_dp<df::FlushOverlaysMessage>(std::move(overlays)), df::MessagePriority::Normal);

  df::TUserMarksRenderData marks;
  for (auto const & key : {neighborKey, oldKey, newKey, oldKey})
    marks.emplace_back(state, nullptr, key);
  queue.PushMessage(make_unique_dp<df::FlushUserMarksMessage>(std::move(marks)), df::MessagePriority::Normal);

  FilterCancelled(queue);
  oldKey.CancelRead();
  FilterCancelled(queue);

  auto const overlayMessage = queue.PopMessage(false);
  TEST(overlayMessage != nullptr, ());
  TEST(overlayMessage->GetType() == df::Message::Type::FlushOverlays, ());
  auto const & retainedOverlays = ref_ptr<df::FlushOverlaysMessage>(make_ref(overlayMessage))->AcceptRenderData();
  TEST_EQUAL(retainedOverlays.size(), 2, ());
  TEST_EQUAL(retainedOverlays[0].m_tileKey.m_x, 0, ());
  TEST_EQUAL(retainedOverlays[1].m_tileKey.m_x, 1, ());
  for (auto const & data : retainedOverlays)
    TEST(!data.m_tileKey.IsCancelled(), ());

  auto const marksMessage = queue.PopMessage(false);
  TEST(marksMessage != nullptr, ());
  TEST(marksMessage->GetType() == df::Message::Type::FlushUserMarks, ());
  auto const & retainedMarks = ref_ptr<df::FlushUserMarksMessage>(make_ref(marksMessage))->AcceptRenderData();
  TEST_EQUAL(retainedMarks.size(), 2, ());
  TEST_EQUAL(retainedMarks[0].m_tileKey.m_x, 1, ());
  TEST_EQUAL(retainedMarks[1].m_tileKey.m_x, 0, ());
  for (auto const & data : retainedMarks)
    TEST(!data.m_tileKey.IsCancelled(), ());
  TEST(queue.PopMessage(false) == nullptr, ());

  newKey.CancelRead();
  neighborKey.CancelRead();
  auto const discard = [](auto const & key) { return key.IsCancelled(); };
  TEST(!df::FilterTileMessage(make_ref(overlayMessage), discard), ());
  TEST(retainedOverlays.empty(), ());
  TEST(!df::FilterTileMessage(make_ref(marksMessage), discard), ());
  TEST(retainedMarks.empty(), ());
}

UNIT_TEST(TileCancellation_TrafficCancelledAfterSweepAndLateArrival)
{
  auto const oldKey = MakeReadKey();
  auto const newKey = MakeReadKey();
  df::MessageQueue queue;
  auto const enqueue = [&](df::TileKey const & key)
  {
    df::TrafficRenderData data(MakeRenderState());
    data.m_tileKey = key;
    queue.PushMessage(make_unique_dp<df::FlushTrafficDataMessage>(std::move(data)), df::MessagePriority::Normal);
  };
  enqueue(oldKey);
  enqueue(newKey);
  FilterCancelled(queue);

  // The consumer must still detect cancellation when it races the preceding queue sweep.
  oldKey.CancelRead();
  auto const cancelledMessage = queue.PopMessage(false);
  TEST(cancelledMessage != nullptr, ());
  TEST(cancelledMessage->GetType() == df::Message::Type::FlushTrafficData, ());
  TEST(ref_ptr<df::FlushTrafficDataMessage>(make_ref(cancelledMessage))->AcceptRenderData().m_tileKey.IsCancelled(),
       ());

  enqueue(oldKey);
  FilterCancelled(queue);
  auto const liveMessage = queue.PopMessage(false);
  TEST(liveMessage != nullptr, ());
  TEST(liveMessage->GetType() == df::Message::Type::FlushTrafficData, ());
  TEST(!ref_ptr<df::FlushTrafficDataMessage>(make_ref(liveMessage))->AcceptRenderData().m_tileKey.IsCancelled(), ());
  TEST(queue.PopMessage(false) == nullptr, ());
}

UNIT_TEST(TileCancellation_OverlappingReadsHaveIndependentBatchers)
{
  auto const oldKey = MakeReadKey();
  auto const newKey = MakeReadKey();
  df::BatchersPool<df::TileKey, df::TileReadKeyComparator> pool(2, [](auto const &, auto const &, auto &&) {}, 16, 16);

  pool.ReserveBatcher(oldKey);
  pool.ReserveBatcher(newKey);
  pool.ReserveBatcher(newKey);
  auto const oldBatcher = pool.GetBatcher(oldKey);
  auto const newBatcher = pool.GetBatcher(newKey);
  TEST(oldBatcher != newBatcher, ());

  oldKey.CancelRead();
  pool.ReleaseBatcher(nullptr, oldKey);
  TEST(pool.GetBatcher(newKey) == newBatcher, ());
  pool.ReleaseBatcher(nullptr, newKey);
  TEST(pool.GetBatcher(newKey) == newBatcher, ());
  pool.ReleaseBatcher(nullptr, newKey);
}

UNIT_TEST(TileCancellation_RequestedViewportDoesNotRetireAcceptedRead)
{
  auto const key = MakeReadKey();
  df::RequestedTiles requested;
  ScreenBase screen;
  requested.Set(screen, false, false, false, df::TTilesCollection{df::TileKey(1, 0, 10)});
  requested.Set(screen, false, false, false, df::TTilesCollection{key});

  TEST(!key.IsCancelled(), ());
  TEST_EQUAL(requested.GetTileCancellationEpoch(), 0, ());
  TEST(!requested.GetTiles().begin()->IsCancelled(), ());

  key.CancelRead();
  requested.NotifyTileCancellation();
  TEST_EQUAL(requested.GetTileCancellationEpoch(), 1, ());
  TEST(key.IsCancelled(), ());
}
}  // namespace tile_cancellation_tests
