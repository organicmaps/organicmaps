#include "testing/testing.hpp"
#include "drape/drape_tests/memory_comparer.hpp"

#include "drape/glconstants.hpp"
#include "drape/batcher.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/shader_def.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "drape/drape_tests/glmock_functions.hpp"

#include "base/stl_add.hpp"

#include "std/bind.hpp"
#include "std/cstring.hpp"
#include "std/function.hpp"
#include "std/bind.hpp"

#include <gmock/gmock.h>

using testing::_;
using testing::Return;
using testing::InSequence;
using testing::Invoke;
using testing::IgnoreResult;
using testing::AnyOf;
using namespace dp;

namespace
{

struct VAOAcceptor
{
  virtual void FlushFullBucket(GLState const & /*state*/, TransferPointer<RenderBucket> bucket)
  {
    m_vao.push_back(MasterPointer<RenderBucket>(bucket));
  }

  vector<MasterPointer<RenderBucket> > m_vao;
};

class BatcherExpectations
{
public:
  BatcherExpectations()
    : m_indexBufferID(1)
    , m_dataBufferID(2)
  {
  }

  template <typename TBatcherCall>
  void RunTest(float * vertexes, uint16_t * indexes,
               uint16_t vertexCount, uint16_t vertexComponentCount,
               uint16_t indexCount, TBatcherCall const & fn)
  {
    int const vertexSize = vertexCount * vertexComponentCount;
    MemoryComparer const dataCmp(vertexes, vertexSize * sizeof(float));
    MemoryComparer const indexCmp(indexes, indexCount * sizeof(uint16_t));

    ExpectBufferCreation(vertexSize, indexCount, indexCmp, dataCmp);

    GLState state(0, GLState::GeometryLayer);

    BindingInfo binding(1);
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = vertexComponentCount;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    AttributeProvider provider(1, vertexCount);
    provider.InitStream(0, binding, MakeStackRefPointer(vertexes));

    VAOAcceptor vaoAcceptor;
    Batcher batcher;
    batcher.StartSession(bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
    fn(&batcher, state, MakeStackRefPointer(&provider));
    batcher.EndSession();

    ExpectBufferDeletion();

    for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
      vaoAcceptor.m_vao[i].Destroy();
  }

  void ExpectBufferCreation(uint16_t vertexCount, uint16_t indexCount,
                            MemoryComparer const & indexCmp, MemoryComparer const & vertexCmp)
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
    EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, indexCount * sizeof(unsigned short), _, gl_const::GLDynamicDraw))
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
  uint32_t m_indexBufferID;
  uint32_t m_dataBufferID;
};

} // namespace

UNIT_TEST(BatchLists_Test)
{
  int const VERTEX_COUNT = 12;
  int const FLOAT_COUNT = 3 * 12; // 3 component on each vertex
  float data[FLOAT_COUNT];
  for (int i = 0; i < FLOAT_COUNT; ++i)
    data[i] = (float)i;

  unsigned short indexes[VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT; ++i)
    indexes[i] = i;

  BatcherExpectations expectations;
  auto fn = [](Batcher * batcher, GLState const & state, RefPointer<AttributeProvider> p)
  {
    batcher->InsertTriangleList(state, p);
  };
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, VERTEX_COUNT, fn);
}

UNIT_TEST(BatchListOfStript_4stride)
{
  int const VERTEX_COUNT = 12;
  int const INDEX_COUNT = 18;

  float data[3 * VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT * 3; ++i)
    data[i] = (float)i;

  unsigned short indexes[INDEX_COUNT] =
    { 0, 1, 2, 1, 3, 2, 4, 5, 6, 5, 7, 6, 8, 9, 10, 9, 11, 10};

  BatcherExpectations expectations;
  auto fn = [](Batcher * batcher, GLState const & state, RefPointer<AttributeProvider> p)
  {
    batcher->InsertListOfStrip(state, p, dp::Batcher::VertexPerQuad);
  };

  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, fn);
}

UNIT_TEST(BatchListOfStript_5stride)
{
  int const VERTEX_COUNT = 15;
  int const INDEX_COUNT = 27;

  float data[3 * VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT * 3; ++i)
    data[i] = (float)i;

  unsigned short indexes[INDEX_COUNT] =
    { 0, 1, 2,
      1, 3, 2,
      2, 3, 4,
      5, 6, 7,
      6, 8, 7,
      7, 8, 9,
      10, 11, 12,
      11, 13, 12,
      12, 13, 14 };

  BatcherExpectations expectations;
  auto fn = [](Batcher * batcher, GLState const & state, RefPointer<AttributeProvider> p)
  {
    batcher->InsertListOfStrip(state, p, 5);
  };
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, fn);
}

UNIT_TEST(BatchListOfStript_6stride)
{
  int const VERTEX_COUNT = 18;
  int const INDEX_COUNT = 36;

  float data[3 * VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT * 3; ++i)
    data[i] = (float)i;

  unsigned short indexes[INDEX_COUNT] =
    { 0, 1, 2,
      1, 3, 2,
      2, 3, 4,
      3, 5, 4,
      6, 7, 8,
      7, 9, 8,
      8, 9, 10,
      9, 11, 10,
      12, 13, 14,
      13, 15, 14,
      14, 15, 16,
      15, 17, 16};

  BatcherExpectations expectations;
  auto fn = [](Batcher * batcher, GLState const & state, RefPointer<AttributeProvider> p)
  {
    batcher->InsertListOfStrip(state, p, 6);
  };
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, fn);
}

namespace
{
  class PartialBatcherTest
  {
  public:
    struct BufferNode
    {
      BufferNode(uint32_t indexByteCount, uint32_t vertexByteCount,
                 void * indexData, void * vertexData)
        : m_indexByteCount(indexByteCount)
        , m_vertexByteCount(vertexByteCount)
        , m_indexData(indexData)
        , m_vertexData(vertexData)
        , m_indexBufferID(0)
        , m_vertexBufferID(0)
      {
      }

      uint32_t m_indexByteCount;
      uint32_t m_vertexByteCount;
      void * m_indexData;
      void * m_vertexData;
      uint32_t m_indexBufferID;
      uint32_t m_vertexBufferID;
    };

    PartialBatcherTest()
      : m_bufferIDCounter(1)
    {
    }

    ~PartialBatcherTest()
    {
      for_each(m_comparators.begin(), m_comparators.end(), DeleteFunctor());
    }

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
      for (size_t i = 0; i < m_nodes.size(); ++i)
      {
        EXPECTGL(glBindBuffer(0, gl_const::GLElementArrayBuffer));
        EXPECTGL(glDeleteBuffer(m_nodes[i].m_indexBufferID));
        EXPECTGL(glBindBuffer(0, gl_const::GLArrayBuffer));
        EXPECTGL(glDeleteBuffer(m_nodes[i].m_vertexBufferID));
      }
    }

  private:
    uint32_t m_bufferIDCounter;

    vector<BufferNode> m_nodes;
    vector<MemoryComparer *> m_comparators;
  };
}

UNIT_TEST(BatchListOfStript_partial)
{
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
    vertexData[i] = (float)i;

  uint16_t indexData[IndexCount] =
    { 0, 1, 2,
      1, 3, 2,
      4, 5, 6,
      5, 7, 6,
      8, 9, 10,
      9, 11, 10,
      0, 1, 2, // start new buffer
      1, 3, 2};

  PartialBatcherTest::BufferNode node1(FirstBufferIndexPortion * sizeof(uint16_t),
                                       FirstBufferVertexPortion * ComponentCount * sizeof(float),
                                       indexData, vertexData);

  PartialBatcherTest::BufferNode node2(SecondBufferIndexPortion * sizeof(uint16_t),
                                       SecondBufferVertexPortion * ComponentCount * sizeof(float),
                                       indexData + FirstBufferIndexPortion,
                                       vertexData + FirstBufferVertexPortion * ComponentCount);

  typedef pair<uint32_t, uint32_t> IndexVertexCount;
  vector<IndexVertexCount> srcData;
  srcData.push_back(make_pair(30, 12));
  srcData.push_back(make_pair(30, 13));
  srcData.push_back(make_pair(18, 30));
  srcData.push_back(make_pair(19, 30));

  for (size_t i = 0; i < srcData.size(); ++i)
  {
    InSequence seq;
    PartialBatcherTest test;
    test.AddBufferNode(node1);
    test.AddBufferNode(node2);
    test.CloseExpection();

    GLState state(0, GLState::GeometryLayer);

    BindingInfo binding(1);
    BindingDecl & decl = binding.GetBindingDecl(0);
    decl.m_attributeName = "position";
    decl.m_componentCount = ComponentCount;
    decl.m_componentType = gl_const::GLFloatType;
    decl.m_offset = 0;
    decl.m_stride = 0;

    AttributeProvider provider(1, VertexCount);
    provider.InitStream(0, binding, MakeStackRefPointer(vertexData));

    VAOAcceptor vaoAcceptor;
    Batcher batcher(srcData[i].first, srcData[i].second);
    batcher.StartSession(bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
    batcher.InsertListOfStrip(state, MakeStackRefPointer(&provider), 4);
    batcher.EndSession();

    for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
      vaoAcceptor.m_vao[i].Destroy();
  }
}
