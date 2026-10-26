#include "testing/testing.hpp"

#include "drape/drape_tests/gl_mock_functions.hpp"
#include "drape/drape_tests/memory_comparer.hpp"
#include "drape/drape_tests/testing_graphics_context.hpp"

#include "drape/batcher.hpp"
#include "drape/batcher_helpers.hpp"
#include "drape/gl_constants.hpp"
#include "drape/index_storage.hpp"

#include "base/stl_helpers.hpp"

#include <cstring>
#include <functional>
#include <vector>

#include <gmock/gmock.h>

using testing::_;
using testing::AnyOf;
using testing::IgnoreResult;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using namespace dp;
using namespace std::placeholders;

namespace terrain_grid_batch_tests
{
UNIT_TEST(TerrainGridBatch_KeepsRowWindingAcrossBufferChange)
{
  struct Callbacks : dp::BatchCallbacks
  {
    uint32_t m_vertices = 4, m_indices = 12, m_changes = 0;
    std::vector<uint8_t> m_storage;
    std::vector<uint32_t> m_uploaded, m_result;
    void FlushData(ref_ptr<dp::GraphicsContext>, dp::BindingInfo const &, void const *, uint32_t count) override
    {
      m_uploaded.push_back(count);
      m_vertices -= count;
    }
    void * GetIndexStorage(uint32_t count, uint32_t & start) override
    {
      start = 0;
      m_storage.resize(count * dp::IndexStorage::SizeOfIndex());
      m_indices -= count;
      return m_storage.data();
    }
    void SubmitIndices(ref_ptr<dp::GraphicsContext>) override
    {
      auto const size = dp::IndexStorage::SizeOfIndex();
      for (size_t i = 0; i < m_storage.size(); i += size)
      {
        uint32_t index = 0;
        std::memcpy(&index, m_storage.data() + i, size);
        m_result.push_back(index);
      }
    }
    uint32_t GetAvailableVertexCount() const override { return m_vertices; }
    uint32_t GetAvailableIndexCount() const override { return m_indices; }
    void ChangeBuffer(ref_ptr<dp::GraphicsContext>) override
    {
      ++m_changes;
      m_vertices = 16;
      m_indices = 64;
    }
  } callbacks;
  float points[] = {0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 2, 1, 2, 0};
  dp::BindingInfo binding(1);
  auto & decl = binding.GetBindingDecl(0);
  decl.m_attributeName = "position";
  decl.m_componentCount = 2;
  decl.m_componentType = gl_const::GLFloatType;
  decl.m_offset = decl.m_stride = 0;
  dp::AttributeProvider provider(1, 7);
  provider.InitStream(0, binding, make_ref(points));
  dp::TriangleStripBatch batch(callbacks, false /* canDivide */);
  // The generic Batcher keeps the batch's own restriction (no overlay handle here).
  batch.SetCanDivideStreams(batch.CanDivideStreams());
  batch.BatchData(nullptr, make_ref(&provider));
  TEST_EQUAL(callbacks.m_changes, 1, ());
  TEST_EQUAL(callbacks.m_uploaded, std::vector<uint32_t>{7}, ("The row must move intact to the fresh buffer"));
  TEST_EQUAL(callbacks.m_result, (std::vector<uint32_t>{0, 1, 2, 1, 3, 2, 2, 3, 4, 3, 5, 4, 4, 5, 6}), ());
  for (size_t t = 3; t < callbacks.m_result.size(); t += 3)
  {
    auto const a = callbacks.m_result[t], b = callbacks.m_result[t + 1], c = callbacks.m_result[t + 2];
    float const cross = (points[2 * b] - points[2 * a]) * (points[2 * c + 1] - points[2 * a + 1]) -
                        (points[2 * b + 1] - points[2 * a + 1]) * (points[2 * c] - points[2 * a]);
    TEST_LESS(cross, 0.0f, (t, "Nondegenerate grid triangles must be CW"));
  }
}
}  // namespace terrain_grid_batch_tests

namespace
{
struct VAOAcceptor
{
  virtual void FlushFullBucket(RenderState const & /* state */, drape_ptr<RenderBucket> && bucket)
  {
    m_vao.push_back(std::move(bucket));
  }

  std::vector<drape_ptr<RenderBucket>> m_vao;
};

class TestExtension : public dp::BaseRenderStateExtension
{
public:
  bool Less(ref_ptr<dp::BaseRenderStateExtension> other) const override { return false; }
  bool Equal(ref_ptr<dp::BaseRenderStateExtension> other) const override { return true; }
};

class BatcherExpectations
{
public:
  BatcherExpectations() : m_indexBufferID(1), m_dataBufferID(2) {}

  template <typename TBatcherCall>
  void RunTest(float * vertexes, void * indexes, uint32_t vertexCount, uint8_t vertexComponentCount,
               uint32_t indexCount, TBatcherCall const & fn)
  {
    uint32_t const vertexSize = vertexCount * vertexComponentCount;
    MemoryComparer const dataCmp(vertexes, vertexSize * sizeof(float));
    MemoryComparer const indexCmp(indexes, indexCount * dp::IndexStorage::SizeOfIndex());

    ExpectBufferCreation(vertexSize, indexCount, indexCmp, dataCmp);

    auto renderState = make_unique_dp<TestExtension>();
    auto state = RenderState(0, make_ref(renderState));

    BindingInfo binding(1);
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = vertexComponentCount;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    AttributeProvider provider(1, vertexCount);
    provider.InitStream(0, binding, make_ref(vertexes));

    TestingGraphicsContext context;
    VAOAcceptor vaoAcceptor;
    Batcher batcher(65000, 65000);
    batcher.StartSession(std::bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
    fn(&batcher, state, make_ref(&provider));
    batcher.EndSession(make_ref(&context));

    ExpectBufferDeletion();

    for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
      vaoAcceptor.m_vao[i].reset();
  }

  void ExpectBufferCreation(uint32_t vertexCount, uint32_t indexCount, MemoryComparer const & indexCmp,
                            MemoryComparer const & vertexCmp)
  {
    InSequence seq;

    // data buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(m_dataBufferID));
    EXPECTGL(glBindBuffer(m_dataBufferID, gl_const::GLArrayBuffer));
    EXPECTGL(glBufferData(gl_const::GLArrayBuffer, vertexCount * sizeof(float), _, gl_const::GLDynamicDraw))
        .WillOnce(Invoke(&vertexCmp, &MemoryComparer::cmpSubBuffer));

    // Index buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(m_indexBufferID));
    EXPECTGL(glBindBuffer(m_indexBufferID, gl_const::GLElementArrayBuffer));
    EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, indexCount * dp::IndexStorage::SizeOfIndex(), _,
                          gl_const::GLDynamicDraw))
        .WillOnce(Invoke(&indexCmp, &MemoryComparer::cmpSubBuffer));

    EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
    EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
  }

  void ExpectBufferDeletion()
  {
    InSequence seq;
    EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
    EXPECTGL(glDeleteBuffer(m_indexBufferID));
    EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
    EXPECTGL(glDeleteBuffer(m_dataBufferID));
  }

private:
  uint32_t m_indexBufferID = 0;
  uint32_t m_dataBufferID = 0;
};

}  // namespace

UNIT_TEST(BatchLists_Test)
{
  TestingGraphicsContext context;
  uint32_t const kVerticesCount = 12;
  uint32_t const kFloatsCount = 3 * 12;  // 3 component on each vertex.
  float data[kFloatsCount];
  for (uint32_t i = 0; i < kFloatsCount; ++i)
    data[i] = static_cast<float>(i);

  std::vector<uint32_t> indexesRaw(kVerticesCount);
  for (uint32_t i = 0; i < kVerticesCount; ++i)
    indexesRaw[i] = i;
  dp::IndexStorage indexes(std::move(indexesRaw));

  BatcherExpectations expectations;
  auto fn = [&context](Batcher * batcher, RenderState const & state, ref_ptr<AttributeProvider> p)
  { batcher->InsertTriangleList(make_ref(&context), state, p); };
  expectations.RunTest(data, indexes.GetRaw(), kVerticesCount, 3, kVerticesCount, fn);
}

UNIT_TEST(BatchListOfStript_4stride)
{
  TestingGraphicsContext context;
  uint32_t const kVerticesCount = 12;
  uint32_t const kIndicesCount = 18;

  float data[3 * kVerticesCount];
  for (uint32_t i = 0; i < kVerticesCount * 3; ++i)
    data[i] = static_cast<float>(i);

  std::vector<uint32_t> indexesRaw = {0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6, 8, 9, 10, 9, 11, 10};
  dp::IndexStorage indexes(std::move(indexesRaw));

  BatcherExpectations expectations;
  auto fn = [&context](Batcher * batcher, RenderState const & state, ref_ptr<AttributeProvider> p)
  { batcher->InsertListOfStrip(make_ref(&context), state, p, dp::Batcher::VertexPerQuad); };

  expectations.RunTest(data, indexes.GetRaw(), kVerticesCount, 3, kIndicesCount, fn);
}

UNIT_TEST(BatchListOfStript_5stride)
{
  TestingGraphicsContext context;
  uint32_t const kVerticesCount = 15;
  uint32_t const kIndicesCount = 27;

  float data[3 * kVerticesCount];
  for (uint32_t i = 0; i < kVerticesCount * 3; ++i)
    data[i] = static_cast<float>(i);

  std::vector<uint32_t> indexesRaw = {0, 1, 2, 1, 3,  2,  2,  3,  4,  5,  6,  7,  6, 8,
                                      7, 7, 8, 9, 10, 11, 12, 11, 13, 12, 12, 13, 14};
  dp::IndexStorage indexes(std::move(indexesRaw));

  BatcherExpectations expectations;
  auto fn = [&context](Batcher * batcher, RenderState const & state, ref_ptr<AttributeProvider> p)
  { batcher->InsertListOfStrip(make_ref(&context), state, p, 5); };
  expectations.RunTest(data, indexes.GetRaw(), kVerticesCount, 3, kIndicesCount, fn);
}

UNIT_TEST(BatchListOfStript_6stride)
{
  TestingGraphicsContext context;
  uint32_t const kVerticesCount = 18;
  uint32_t const kIndicesCount = 36;

  float data[3 * kVerticesCount];
  for (uint32_t i = 0; i < kVerticesCount * 3; ++i)
    data[i] = static_cast<float>(i);

  std::vector<uint32_t> indexesRaw = {0, 1, 2,  1, 3,  2,  2,  3,  4,  3,  5,  4,  6,  7,  8,  7,  9,  8,
                                      8, 9, 10, 9, 11, 10, 12, 13, 14, 13, 15, 14, 14, 15, 16, 15, 17, 16};
  dp::IndexStorage indexes(std::move(indexesRaw));

  BatcherExpectations expectations;
  auto fn = [&context](Batcher * batcher, RenderState const & state, ref_ptr<AttributeProvider> p)
  { batcher->InsertListOfStrip(make_ref(&context), state, p, 6); };
  expectations.RunTest(data, indexes.GetRaw(), kVerticesCount, 3, kIndicesCount, fn);
}

namespace
{
class PartialBatcherTest
{
public:
  struct BufferNode
  {
    BufferNode(uint32_t indexByteCount, uint32_t vertexByteCount, void * indexData, void * vertexData)
      : m_indexByteCount(indexByteCount)
      , m_vertexByteCount(vertexByteCount)
      , m_indexData(indexData)
      , m_vertexData(vertexData)
      , m_indexBufferID(0)
      , m_vertexBufferID(0)
    {}

    uint32_t m_indexByteCount;
    uint32_t m_vertexByteCount;
    void * m_indexData;
    void * m_vertexData;
    uint32_t m_indexBufferID;
    uint32_t m_vertexBufferID;
  };

  PartialBatcherTest() = default;

  ~PartialBatcherTest() { std::for_each(m_comparators.begin(), m_comparators.end(), base::DeleteFunctor()); }

  void AddBufferNode(BufferNode const & node)
  {
    m_nodes.push_back(node);
    BufferNode & currentNode = m_nodes.back();
    currentNode.m_indexBufferID = m_bufferIDCounter++;
    currentNode.m_vertexBufferID = m_bufferIDCounter++;

    // data buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(currentNode.m_vertexBufferID));
    EXPECTGL(glBindBuffer(currentNode.m_vertexBufferID, gl_const::GLArrayBuffer));

    m_comparators.push_back(new MemoryComparer(currentNode.m_vertexData, currentNode.m_vertexByteCount));
    MemoryComparer * vertexComparer = m_comparators.back();

    EXPECTGL(glBufferData(gl_const::GLArrayBuffer, currentNode.m_vertexByteCount, _, gl_const::GLDynamicDraw))
        .WillOnce(Invoke(vertexComparer, &MemoryComparer::cmpSubBuffer));

    // Index buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(currentNode.m_indexBufferID));
    EXPECTGL(glBindBuffer(currentNode.m_indexBufferID, gl_const::GLElementArrayBuffer));

    m_comparators.push_back(new MemoryComparer(currentNode.m_indexData, currentNode.m_indexByteCount));
    MemoryComparer * indexComparer = m_comparators.back();

    EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, currentNode.m_indexByteCount, _, gl_const::GLDynamicDraw))
        .WillOnce(Invoke(indexComparer, &MemoryComparer::cmpSubBuffer));

    EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
    EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
  }

  void CloseExpection()
  {
    for (auto const & node : m_nodes)
    {
      EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
      EXPECTGL(glDeleteBuffer(node.m_indexBufferID));
      EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
      EXPECTGL(glDeleteBuffer(node.m_vertexBufferID));
    }
  }

private:
  uint32_t m_bufferIDCounter = 1;

  std::vector<BufferNode> m_nodes;
  std::vector<MemoryComparer *> m_comparators;
};
}  // namespace

UNIT_TEST(BatchListOfStript_partial)
{
  TestingGraphicsContext context;
  uint32_t const VertexCount = 16;
  uint32_t const ComponentCount = 3;
  uint32_t const VertexArraySize = VertexCount * ComponentCount;
  uint32_t const IndexCount = 24;

  uint32_t const FirstBufferVertexPortion = 12;
  uint32_t const SecondBufferVertexPortion = VertexCount - FirstBufferVertexPortion;
  uint32_t const FirstBufferIndexPortion = 18;
  uint32_t const SecondBufferIndexPortion = IndexCount - FirstBufferIndexPortion;

  float vertexData[VertexArraySize];
  for (uint32_t i = 0; i < VertexArraySize; ++i)
    vertexData[i] = static_cast<float>(i);

  std::vector<uint32_t> indexDataRaw = {0, 1, 2, 1,  3, 2,  4,  5, 6, 5, 7,
                                        6, 8, 9, 10, 9, 11, 10, 0, 1, 2,  // start new buffer
                                        1, 3, 2};
  dp::IndexStorage indexData(std::move(indexDataRaw));

  PartialBatcherTest::BufferNode node1(FirstBufferIndexPortion * dp::IndexStorage::SizeOfIndex(),
                                       FirstBufferVertexPortion * ComponentCount * sizeof(float), indexData.GetRaw(),
                                       vertexData);

  PartialBatcherTest::BufferNode node2(SecondBufferIndexPortion * dp::IndexStorage::SizeOfIndex(),
                                       SecondBufferVertexPortion * ComponentCount * sizeof(float),
                                       indexData.GetRaw(FirstBufferIndexPortion),
                                       vertexData + FirstBufferVertexPortion * ComponentCount);

  using IndexVertexCount = std::pair<uint32_t, uint32_t>;
  std::vector<IndexVertexCount> srcData;
  srcData.emplace_back(30, 12);
  srcData.emplace_back(30, 13);
  srcData.emplace_back(18, 30);
  srcData.emplace_back(19, 30);

  for (size_t i = 0; i < srcData.size(); ++i)
  {
    InSequence seq;
    PartialBatcherTest test;
    test.AddBufferNode(node1);
    test.AddBufferNode(node2);
    test.CloseExpection();

    auto renderState = make_unique_dp<TestExtension>();
    auto state = RenderState(0, make_ref(renderState));

    BindingInfo binding(1);
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = ComponentCount;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    AttributeProvider provider(1, VertexCount);
    provider.InitStream(0, binding, make_ref(vertexData));

    VAOAcceptor vaoAcceptor;
    Batcher batcher(srcData[i].first, srcData[i].second);
    batcher.StartSession(std::bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
    batcher.InsertListOfStrip(make_ref(&context), state, make_ref(&provider), 4);
    batcher.EndSession(make_ref(&context));

    for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
      vaoAcceptor.m_vao[i].reset();
  }
}

UNIT_TEST(BatchLineStrip_split)
{
  // A line strip longer than the whole buffer must split with the boundary vertex
  // repeated (the long dynamic isolines case), not overflow the index buffer.
  TestingGraphicsContext context;
  uint32_t const VertexCount = 16;
  uint32_t const ComponentCount = 3;
  uint32_t const VertexArraySize = VertexCount * ComponentCount;

  // Batcher(20 indices, 100 vertices): the first chunk takes 11 vertices / 20 indices
  // (the full index buffer), the second continues from the repeated vertex 10.
  uint32_t const FirstChunkVertices = 11;
  uint32_t const SecondChunkVertices = VertexCount - FirstChunkVertices + 1;

  float vertexData[VertexArraySize];
  for (uint32_t i = 0; i < VertexArraySize; ++i)
    vertexData[i] = static_cast<float>(i);

  std::vector<uint32_t> indexDataRaw = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10,
                                        // start new buffer
                                        0, 1, 1, 2, 2, 3, 3, 4, 4, 5};
  dp::IndexStorage indexData(std::move(indexDataRaw));

  InSequence seq;
  PartialBatcherTest test;
  test.AddBufferNode(PartialBatcherTest::BufferNode(20 * dp::IndexStorage::SizeOfIndex(),
                                                    FirstChunkVertices * ComponentCount * sizeof(float),
                                                    indexData.GetRaw(), vertexData));
  test.AddBufferNode(PartialBatcherTest::BufferNode(
      10 * dp::IndexStorage::SizeOfIndex(), SecondChunkVertices * ComponentCount * sizeof(float), indexData.GetRaw(20),
      vertexData + (FirstChunkVertices - 1) * ComponentCount));
  test.CloseExpection();

  auto renderState = make_unique_dp<TestExtension>();
  auto state = RenderState(0, make_ref(renderState));

  BindingInfo binding(1);
  BindingDecl & decl = binding.GetBindingDecl(0);
  decl.m_attributeName = "position";
  decl.m_componentCount = ComponentCount;
  decl.m_componentType = gl_const::GLFloatType;
  decl.m_offset = 0;
  decl.m_stride = 0;

  AttributeProvider provider(1, VertexCount);
  provider.InitStream(0, binding, make_ref(vertexData));

  VAOAcceptor vaoAcceptor;
  Batcher batcher(20 /* indexBufferSize */, 100 /* vertexBufferSize */);
  batcher.StartSession(std::bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
  batcher.InsertLineStrip(make_ref(&context), state, make_ref(&provider));
  batcher.EndSession(make_ref(&context));

  for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
    vaoAcceptor.m_vao[i].reset();
}
