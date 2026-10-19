#include "testing/testing.hpp"

#include "drape_frontend/base_renderer.hpp"
#include "drape_frontend/engine_context.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/tile_info.hpp"

#include <numeric>
#include <vector>

namespace shape_batch_tests
{
class EmptyRoutine : public threads::IRoutine
{
public:
  void Do() override {}
};

class CountedShape : public df::MapShape
{
public:
  CountedShape(size_t id, size_t & destroyed) : m_id(id), m_destroyed(destroyed) {}
  ~CountedShape() override { ++m_destroyed; }
  void Draw(ref_ptr<dp::GraphicsContext>, ref_ptr<dp::Batcher>, ref_ptr<dp::TextureManager>) const override {}

  size_t const m_id;

private:
  size_t & m_destroyed;
};

class ShapeReceiver : public df::BaseRenderer
{
public:
  explicit ShapeReceiver(df::ThreadsCommutator & commutator)
    : BaseRenderer(df::ThreadsCommutator::ResourceUploadThread,
                   Params(dp::ApiVersion::Invalid, make_ref(&commutator), nullptr, nullptr, {}))
  {
    StartThread();
  }
  ~ShapeReceiver() override { StopThread(); }

  void Drain()
  {
    while (ProcessSingleMessage(false))
    {}
  }

  std::vector<df::Message::Type> m_types;
  std::vector<size_t> m_batchSizes;
  std::vector<size_t> m_shapeIds;

private:
  std::unique_ptr<threads::IRoutine> CreateRoutine() override { return std::make_unique<EmptyRoutine>(); }
  void RenderFrame() override {}
  void OnContextCreate() override {}
  void OnContextDestroy() override {}
  void AcceptMessage(ref_ptr<df::Message> message) override
  {
    auto const type = message->GetType();
    m_types.push_back(type);
    if (type == df::Message::Type::MapShapeReaded || type == df::Message::Type::OverlayMapShapeReaded)
    {
      auto const & shapes = ref_ptr<df::MapShapeReadedMessage>(message)->GetShapes();
      m_batchSizes.push_back(shapes.size());
      for (auto const & shape : shapes)
        m_shapeIds.push_back(ref_ptr<CountedShape>(make_ref(shape))->m_id);
    }
  }
};

df::EngineContext MakeContext(df::ThreadsCommutator & commutator)
{
  df::TileKey const key(1, 2, 15);
  return {key, make_ref(&commutator), nullptr, nullptr, {}, false, false, false, 0, dp::BackgroundMode::Default, 1.0f};
}

df::TMapShapes MakeShapes(size_t begin, size_t count, size_t & destroyed)
{
  df::TMapShapes shapes;
  for (size_t i = begin; i < begin + count; ++i)
    shapes.push_back(make_unique_dp<CountedShape>(i, destroyed));
  return shapes;
}

UNIT_TEST(ShapeBatch_PreservesOrderAndFlushesAtBoundaries)
{
  using Type = df::Message::Type;
  size_t destroyed = 0;
  df::ThreadsCommutator commutator;
  ShapeReceiver receiver(commutator);
  auto context = MakeContext(commutator);
  context.BeginReadTile();
  context.Flush(MakeShapes(0, 3, destroyed));
  context.Flush(MakeShapes(3, 130, destroyed));
  receiver.Drain();
  TEST_EQUAL(receiver.m_batchSizes, (std::vector<size_t>{64, 64}),
             ("Full batches are posted while the partial tail remains buffered"));

  context.FlushOverlays(MakeShapes(133, 65, destroyed));
  context.Flush(MakeShapes(198, 1, destroyed));
  context.EndReadTile();
  receiver.Drain();
  std::vector<Type> const expectedTypes{Type::TileReadStarted, Type::MapShapeReaded,        Type::MapShapeReaded,
                                        Type::MapShapeReaded,  Type::OverlayMapShapeReaded, Type::MapShapeReaded,
                                        Type::TileReadEnded};
  TEST_EQUAL(receiver.m_types, expectedTypes, ());
  TEST_EQUAL(receiver.m_batchSizes, (std::vector<size_t>{64, 64, 5, 65, 1}),
             ("Flush geometry tails before overlays and read completion; keep overlays whole"));
  std::vector<size_t> expectedIds(199);
  std::iota(expectedIds.begin(), expectedIds.end(), 0);
  TEST_EQUAL(receiver.m_shapeIds, expectedIds, ());
}

UNIT_TEST(ShapeBatch_CancellationDropsPendingAndIncomingShapesBeforeEnqueue)
{
  size_t destroyed = 0;
  df::ThreadsCommutator commutator;
  ShapeReceiver receiver(commutator);
  auto context = make_unique_dp<df::EngineContext>(MakeContext(commutator));
  auto const read = context.get();
  df::TileInfo tile(std::move(context));
  read->BeginReadTile();
  read->Flush(MakeShapes(0, 7, destroyed));
  TEST(!tile.IsCancelled(), ());
  tile.Cancel();
  TEST(tile.IsCancelled(), ());
  TEST(read->IsCancelled(), ());

  auto geometry = MakeShapes(7, 130, destroyed);
  read->Flush(std::move(geometry));
  TEST(geometry.empty(), ());
  auto overlays = MakeShapes(137, 9, destroyed);
  read->FlushOverlays(std::move(overlays));
  TEST(overlays.empty(), ());
  TEST_EQUAL(destroyed, 146, ());
  read->EndReadTile();
  receiver.Drain();
  TEST_EQUAL(receiver.m_types,
             (std::vector<df::Message::Type>{df::Message::Type::TileReadStarted, df::Message::Type::TileReadEnded}),
             ("The receiver does not filter canceled messages"));
}

}  // namespace shape_batch_tests
