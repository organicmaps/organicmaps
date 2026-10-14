#include "testing/testing.hpp"

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/map_data_provider.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/metaline_manager.hpp"
#include "drape_frontend/read_manager.hpp"
#include "drape_frontend/tile_info.hpp"
#include "drape_frontend/visual_params.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

namespace read_cancellation_tests
{
class EmptyRoutine : public threads::IRoutine
{
public:
  void Do() override {}
};

class CountedShape : public df::MapShape
{
public:
  CountedShape(int id, size_t & destroyed) : m_id(id), m_destroyed(destroyed) {}
  ~CountedShape() override { ++m_destroyed; }
  void Draw(ref_ptr<dp::GraphicsContext>, ref_ptr<dp::Batcher>, ref_ptr<dp::TextureManager>) const override {}

  int const m_id;

private:
  size_t & m_destroyed;
};

class ReadReceiver : public df::BaseRenderer
{
public:
  explicit ReadReceiver(df::ThreadsCommutator & commutator)
    : BaseRenderer(df::ThreadsCommutator::ResourceUploadThread,
                   Params(dp::ApiVersion::Invalid, make_ref(&commutator), nullptr, nullptr, {}))
  {
    StartThread();
  }
  ~ReadReceiver() override { StopThread(); }

  void Sweep() { InstantMessageFilter(df::MapShapeReadedMessage::IsCancelledMessage); }
  void Drain()
  {
    while (ProcessSingleMessage(false))
    {}
  }

  std::vector<df::Message::Type> m_types;
  std::vector<df::TileKey> m_started;
  std::vector<df::TileKey> m_ended;
  std::vector<int> m_shapeIds;
  size_t m_rejected = 0;

private:
  std::unique_ptr<threads::IRoutine> CreateRoutine() override { return std::make_unique<EmptyRoutine>(); }
  void RenderFrame() override {}
  void OnContextCreate() override {}
  void OnContextDestroy() override {}
  void AcceptMessage(ref_ptr<df::Message> message) override
  {
    // The backend uses this same gate for a producer finishing after the sweep.
    if (df::MapShapeReadedMessage::IsCancelledMessage(message))
    {
      ++m_rejected;
      return;
    }
    auto const type = message->GetType();
    m_types.push_back(type);
    if (type == df::Message::Type::TileReadStarted)
      m_started.push_back(ref_ptr<df::TileReadStartMessage>(message)->GetKey());
    else if (type == df::Message::Type::TileReadEnded)
      m_ended.push_back(ref_ptr<df::TileReadEndMessage>(message)->GetKey());
    else if (type == df::Message::Type::MapShapeReaded || type == df::Message::Type::OverlayMapShapeReaded)
      for (auto const & shape : ref_ptr<df::MapShapeReadedMessage>(message)->GetShapes())
        m_shapeIds.push_back(ref_ptr<CountedShape>(make_ref(shape))->m_id);
  }
};

df::EngineContext MakeContext(df::TileKey const & key, df::ThreadsCommutator & commutator)
{
  return {key, make_ref(&commutator), nullptr, nullptr, {}, false, false, false, 0, dp::BackgroundMode::Default, 1.0f};
}

df::TMapShapes MakeShape(int id, size_t & destroyed)
{
  df::TMapShapes shapes;
  shapes.push_back(make_unique_dp<CountedShape>(id, destroyed));
  return shapes;
}

UNIT_TEST(ReadCancellation_SweepDestroysOnlyRetiredPayloadsAndPreservesLifecycle)
{
  using Type = df::Message::Type;
  size_t oldDestroyed = 0;
  size_t newDestroyed = 0;
  df::ThreadsCommutator commutator;
  ReadReceiver receiver(commutator);
  df::TileKey const key(df::TileKey(1, 2, 15), 7, 9);
  {
    auto oldRead = MakeContext(key, commutator);
    auto newRead = MakeContext(key, commutator);
    oldRead.BeginReadTile();
    oldRead.Flush(MakeShape(1, oldDestroyed));
    newRead.BeginReadTile();
    newRead.Flush(MakeShape(2, newDestroyed));
    oldRead.FlushOverlays(MakeShape(3, oldDestroyed));
    commutator.PostMessage(df::ThreadsCommutator::ResourceUploadThread, make_unique_dp<df::FinishReadingMessage>(),
                           df::MessagePriority::Normal);
    newRead.FlushOverlays(MakeShape(4, newDestroyed));
    oldRead.EndReadTile();
    newRead.EndReadTile();

    TEST(!oldRead.IsCancelled(), ());
    TEST(!newRead.IsCancelled(), ());
    oldRead.Cancel();
    TEST(oldRead.IsCancelled(), ());
    TEST(!newRead.IsCancelled(), ("Even equal tile keys must have independent read lifetimes"));
  }
  // Both contexts are gone: the queued messages must retain their cancellation state.
  TEST_EQUAL(oldDestroyed, 0, ());
  TEST_EQUAL(newDestroyed, 0, ());
  receiver.Sweep();
  TEST_EQUAL(oldDestroyed, 2, ("The sweep must free obsolete geometry and overlays immediately"));
  TEST_EQUAL(newDestroyed, 0, ());
  receiver.Drain();
  // The new read's buffered geometry is flushed by its overlay after the control message.
  std::vector<Type> const expected{Type::TileReadStarted, Type::TileReadStarted,       Type::FinishReading,
                                   Type::MapShapeReaded,  Type::OverlayMapShapeReaded, Type::TileReadEnded,
                                   Type::TileReadEnded};
  TEST_EQUAL(receiver.m_types, expected, ());
  TEST_EQUAL(receiver.m_shapeIds, (std::vector<int>{2, 4}), ());
  TEST_EQUAL(receiver.m_rejected, 0, ("Canceled payloads should already have been removed from the queue"));
  TEST_EQUAL(newDestroyed, 2, ());
  TEST_EQUAL(receiver.m_started.size(), 2, ());
  TEST_EQUAL(receiver.m_ended.size(), 2, ());
  for (auto const & tile : receiver.m_started)
    TEST(tile.EqualStrict(key), ());
  for (auto const & tile : receiver.m_ended)
    TEST(tile.EqualStrict(key), ());
}

UNIT_TEST(ReadCancellation_RejectsPayloadsPostedAfterSweep)
{
  using Type = df::Message::Type;
  size_t destroyed = 0;
  df::ThreadsCommutator commutator;
  ReadReceiver receiver(commutator);
  auto const key = df::TileKey(1, 2, 15);
  df::TileReadCancellation cancelled = std::make_shared<std::atomic<bool>>(false);
  auto geometry = make_unique_dp<df::MapShapeReadedMessage>(key, MakeShape(1, destroyed), cancelled);
  auto overlays = make_unique_dp<df::OverlayMapShapeReadedMessage>(key, MakeShape(2, destroyed), cancelled);
  commutator.PostMessage(df::ThreadsCommutator::ResourceUploadThread, make_unique_dp<df::TileReadStartMessage>(key),
                         df::MessagePriority::Normal);
  cancelled->store(true, std::memory_order_relaxed);
  receiver.Sweep();

  // The producer checked cancellation before preparing these payloads, but enqueues after the sweep.
  commutator.PostMessage(df::ThreadsCommutator::ResourceUploadThread, std::move(geometry), df::MessagePriority::Normal);
  commutator.PostMessage(df::ThreadsCommutator::ResourceUploadThread, std::move(overlays), df::MessagePriority::Normal);
  commutator.PostMessage(df::ThreadsCommutator::ResourceUploadThread, make_unique_dp<df::TileReadEndMessage>(key),
                         df::MessagePriority::Normal);
  cancelled.reset();
  TEST_EQUAL(destroyed, 0, ());
  receiver.Drain();
  TEST_EQUAL(receiver.m_rejected, 2, ());
  TEST(receiver.m_shapeIds.empty(), ());
  TEST_EQUAL(destroyed, 2, ());
  TEST_EQUAL(receiver.m_types, (std::vector<Type>{Type::TileReadStarted, Type::TileReadEnded}), ());
}

UNIT_TEST(ReadCancellation_CancelledBufferedTailIsReleasedAtReadEnd)
{
  using Type = df::Message::Type;
  size_t destroyed = 0;
  df::ThreadsCommutator commutator;
  ReadReceiver receiver(commutator);
  auto read = MakeContext(df::TileKey(1, 2, 15), commutator);
  read.BeginReadTile();
  read.Flush(MakeShape(1, destroyed));
  receiver.Drain();
  TEST(receiver.m_shapeIds.empty(), ("A partial batch remains buffered in the read"));

  read.Cancel();
  receiver.Sweep();
  TEST_EQUAL(destroyed, 0, ("The queue sweep cannot own the buffered tail yet"));
  read.EndReadTile();
  TEST_EQUAL(destroyed, 1, ("The producer releases its canceled tail before posting read completion"));
  receiver.Drain();
  TEST_EQUAL(receiver.m_rejected, 0, ());
  TEST_EQUAL(destroyed, 1, ());
  TEST(receiver.m_shapeIds.empty(), ());
  TEST_EQUAL(receiver.m_types, (std::vector<Type>{Type::TileReadStarted, Type::TileReadEnded}), ());
}

UNIT_TEST(ReadCancellation_IndexCancellationPreservesLifecycle)
{
  using Type = df::Message::Type;
  for (bool const cancelBeforeIndex : {true, false})
  {
    df::ThreadsCommutator commutator;
    ReadReceiver receiver(commutator);
    df::TileInfo tile(make_unique_dp<df::EngineContext>(MakeContext(df::TileKey(1, 2, 15), commutator)));
    bool indexRead = false;
    bool featuresRead = false;
    df::MapDataProvider model([&](auto const & callback, auto const &, int)
    {
      indexRead = true;
      callback(FeatureID{});
      tile.Cancel();
    }, [&](auto const &, auto const &) { featuresRead = true; }, [](std::string_view) {
      return true;
    }, [](auto const &, int) {}, [](auto const &, auto) { return false; }, [](auto const &, auto) {});
    if (cancelBeforeIndex)
      tile.Cancel();
    tile.ReadFeatures(model);
    receiver.Drain();
    TEST_EQUAL(indexRead, !cancelBeforeIndex, ());
    TEST(!featuresRead, ());
    TEST_EQUAL(receiver.m_types, (std::vector<Type>{Type::TileReadStarted, Type::TileReadEnded}), ());
  }
}

class ReadCoverageFixture
{
public:
  ReadCoverageFixture()
    : m_receiver(m_commutator)
    , m_model([this](auto const &, auto const &, int) { NotifyRead(); }, [](auto const &, auto const &) {},
              [](std::string_view) { return true; }, [](auto const &, int) {}, [](auto const &, auto) { return false; },
              [](auto const &, auto) {})
    , m_metalines(make_ref(&m_commutator), m_model)
    , m_reader(make_ref(&m_commutator), m_model, false, false, false, dp::BackgroundMode::Default, 0.5f)
  {
    df::VisualParams::Init(1.0, 1024);
    m_screen.OnSize(0, 0, 256, 256);
    m_screen.SetFromRect(m2::AnyRectD(m_a.GetGlobalRect()));
  }
  ~ReadCoverageFixture() { Stop(); }

  void NotifyRead()
  {
    std::lock_guard lock(m_mutex);
    ++m_indexReads;
    m_condition.notify_one();
  }
  void Update(df::TTilesCollection const & tiles)
  {
    m_reader.UpdateCoverage(m_screen, false, false, false, tiles, nullptr, make_ref(&m_metalines));
  }
  bool WaitForReads(size_t count)
  {
    // The provider runs after BeginReadTile posts the corresponding start message.
    std::unique_lock lock(m_mutex);
    bool const ready = m_condition.wait_for(lock, std::chrono::seconds(5), [&] { return m_indexReads >= count; });
    lock.unlock();
    m_receiver.Drain();
    return ready;
  }
  void Stop()
  {
    m_reader.Stop();
    m_metalines.Stop();
    m_receiver.Drain();
  }
  df::TileKey FindRead(df::TileKey const & key) const
  {
    auto const it = std::find(m_receiver.m_started.begin(), m_receiver.m_started.end(), key);
    TEST(it != m_receiver.m_started.end(), (key));
    return *it;
  }
  void CheckLifecycle() const
  {
    auto started = m_receiver.m_started;
    auto ended = m_receiver.m_ended;
    std::sort(started.begin(), started.end(), df::TileKeyStrictComparator{});
    std::sort(ended.begin(), ended.end(), df::TileKeyStrictComparator{});
    TEST_EQUAL(started.size(), ended.size(), ());
    for (size_t i = 0; i < started.size(); ++i)
      TEST(started[i].EqualStrict(ended[i]), ());
  }

  df::TileKey const m_a{1, 1, 15};
  df::TileKey const m_b{2, 1, 15};
  std::mutex m_mutex;
  std::condition_variable m_condition;
  size_t m_indexReads = 0;
  df::ThreadsCommutator m_commutator;
  ReadReceiver m_receiver;
  df::MapDataProvider m_model;
  df::MetalineManager m_metalines;
  df::ReadManager m_reader;
  ScreenBase m_screen;
};

UNIT_CLASS_TEST(ReadCoverageFixture, ReadCancellation_ReentryKeepsNeighborAndAdvancesRevision)
{
  auto const initialRevision = m_reader.GetCancellationRevision();
  Update({m_a, m_b});
  TEST(WaitForReads(2), ());
  auto const oldA = FindRead(m_a);
  TEST_EQUAL(m_reader.GetCancellationRevision(), initialRevision, ());
  Update({m_b});
  TEST_EQUAL(m_reader.GetCancellationRevision(), initialRevision + 1, ());
  TEST(!m_reader.CheckTileKey(m_a), ());
  TEST(m_reader.CheckTileKey(m_b), ());
  Update({m_b});
  TEST_EQUAL(m_reader.GetCancellationRevision(), initialRevision + 1, ("Unchanged coverage needs no sweep"));

  Update({m_a, m_b});
  TEST(WaitForReads(3), ());
  TEST_EQUAL(m_receiver.m_started.size(), 3, ());
  TEST(m_receiver.m_started.back().EqualStrict(oldA), ("Reentry must not change the whole viewport generation"));
  TEST_EQUAL(std::count(m_receiver.m_started.begin(), m_receiver.m_started.end(), m_b), 1, ());
  TEST(m_reader.CheckTileKey(m_a), ());
  TEST(m_reader.CheckTileKey(m_b), ());
  TEST_EQUAL(m_reader.GetCancellationRevision(), initialRevision + 1, ());
  Stop();
  TEST_EQUAL(m_reader.GetCancellationRevision(), initialRevision + 3, ());
  CheckLifecycle();
}

UNIT_CLASS_TEST(ReadCoverageFixture, ReadCancellation_ExplicitInvalidationAdvancesRevision)
{
  Update({m_a, m_b});
  TEST(WaitForReads(2), ());
  auto const revision = m_reader.GetCancellationRevision();
  m_reader.Invalidate({m_a});
  TEST_EQUAL(m_reader.GetCancellationRevision(), revision + 1, ());
  TEST(!m_reader.CheckTileKey(m_a), ());
  TEST(m_reader.CheckTileKey(m_b), ());
  m_reader.Invalidate({m_a});
  TEST_EQUAL(m_reader.GetCancellationRevision(), revision + 1, ("An already removed read must not retire again"));
  m_reader.InvalidateAll();
  TEST_EQUAL(m_reader.GetCancellationRevision(), revision + 2, ());
  TEST(!m_reader.CheckTileKey(m_b), ());
  m_reader.InvalidateAll();
  TEST_EQUAL(m_reader.GetCancellationRevision(), revision + 2, ());
  Stop();
  CheckLifecycle();
}
}  // namespace read_cancellation_tests
