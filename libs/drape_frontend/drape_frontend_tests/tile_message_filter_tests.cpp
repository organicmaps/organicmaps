#include "testing/testing.hpp"

#include "drape_frontend/message_queue.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/tile_message_filter.hpp"

namespace tile_message_filter_tests
{
df::TileKey Key(uint64_t generation)
{
  return df::TileKey(df::TileKey(1, 2, 15), generation, generation);
}

UNIT_TEST(TileMessageFilter_GenerationsAndNotifications)
{
  auto const state = df::CreateRenderState(gpu::Program::Area, df::DepthLayer::GeometryLayer);
  df::MessageQueue queue;
  for (uint64_t const generation : {4, 5, 6})
    queue.PushMessage(make_unique_dp<df::FlushRenderBucketMessage>(Key(generation), state, nullptr),
                      df::MessagePriority::Normal);

  df::TOverlaysRenderData overlays;
  for (uint64_t const generation : {4, 6, 3, 5})
    overlays.emplace_back(Key(generation), state, nullptr);
  queue.PushMessage(make_unique_dp<df::FlushOverlaysMessage>(std::move(overlays)), df::MessagePriority::Normal);

  df::TUserMarksRenderData marks;
  for (uint64_t const generation : {3, 5, 4, 6})
    marks.emplace_back(state, nullptr, Key(generation));
  queue.PushMessage(make_unique_dp<df::FlushUserMarksMessage>(std::move(marks)), df::MessagePriority::Normal);
  queue.PushMessage(make_unique_dp<df::FinishTileReadMessage>(df::TTilesCollection{Key(4)}, false),
                    df::MessagePriority::Normal);

  queue.InstantFilter([](ref_ptr<df::Message> message)
  { return df::FilterTileMessage(message, [](df::TileKey const & key) { return key.m_generation < 5; }); });

  for (uint64_t const generation : {5, 6})
  {
    auto const message = queue.PopMessage(false);
    TEST(message != nullptr, ());
    TEST(message->GetType() == df::Message::Type::FlushTile, ());
    TEST_EQUAL(ref_ptr<df::FlushRenderBucketMessage>(make_ref(message))->GetKey().m_generation, generation, ());
  }
  auto const overlayMessage = queue.PopMessage(false);
  TEST(overlayMessage->GetType() == df::Message::Type::FlushOverlays, ());
  auto const & retainedOverlays = ref_ptr<df::FlushOverlaysMessage>(make_ref(overlayMessage))->AcceptRenderData();
  TEST_EQUAL(retainedOverlays.size(), 2, ());
  TEST_EQUAL(retainedOverlays[0].m_tileKey.m_generation, 6, ());
  TEST_EQUAL(retainedOverlays[1].m_tileKey.m_generation, 5, ());

  auto const marksMessage = queue.PopMessage(false);
  TEST(marksMessage->GetType() == df::Message::Type::FlushUserMarks, ());
  auto const & retainedMarks = ref_ptr<df::FlushUserMarksMessage>(make_ref(marksMessage))->AcceptRenderData();
  TEST_EQUAL(retainedMarks.size(), 2, ());
  TEST_EQUAL(retainedMarks[0].m_tileKey.m_generation, 5, ());
  TEST_EQUAL(retainedMarks[1].m_tileKey.m_generation, 6, ());

  auto const completion = queue.PopMessage(false);
  TEST(completion->GetType() == df::Message::Type::FinishTileRead, ());
  TEST_EQUAL(ref_ptr<df::FinishTileReadMessage>(make_ref(completion))->GetTiles().begin()->m_generation, 4, ());
  TEST(queue.PopMessage(false) == nullptr, ());
}

UNIT_TEST(TileMessageFilter_KeepsEmptyBatchesAndControlMessages)
{
  auto const state = df::CreateRenderState(gpu::Program::Area, df::DepthLayer::GeometryLayer);
  df::TOverlaysRenderData overlays;
  overlays.emplace_back(Key(1), state, nullptr);
  auto overlayMessage = make_unique_dp<df::FlushOverlaysMessage>(std::move(overlays));
  TEST(!df::FilterTileMessage(make_ref(overlayMessage), [](auto const &) { return true; }), ());
  TEST(overlayMessage->AcceptRenderData().empty(), ());

  df::TUserMarksRenderData marks;
  marks.emplace_back(state, nullptr, Key(1));
  auto marksMessage = make_unique_dp<df::FlushUserMarksMessage>(std::move(marks));
  TEST(!df::FilterTileMessage(make_ref(marksMessage), [](auto const &) { return true; }), ());
  TEST(marksMessage->AcceptRenderData().empty(), ());

  auto control = make_unique_dp<df::InvalidateRectMessage>(m2::RectD());
  TEST(!df::FilterTileMessage(make_ref(control),
                              [](auto const &)
  {
    TEST(false, ("Control messages must not be tested for tile obsolescence"));
    return true;
  }),
       ());
}
}  // namespace tile_message_filter_tests
