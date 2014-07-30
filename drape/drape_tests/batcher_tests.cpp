#include "../../testing/testing.hpp"

#include "../glconstants.hpp"
#include "../batcher.hpp"
#include "../gpu_program_manager.hpp"
#include "../shader_def.hpp"
#include "../vertex_array_buffer.hpp"

#include "glmock_functions.hpp"

#include "../../base/stl_add.hpp"

#include "../../std/bind.hpp"
#include "../../std/scoped_ptr.hpp"
#include "../../std/cstring.hpp"
#include "../../std/function.hpp"
#include "../../std/bind.hpp"

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

struct MemoryComparer
{
  void * m_mem;
  int m_size;

  MemoryComparer(void * memory, int size)
    : m_mem(memory)
    , m_size(size)
  {
  }

  void cmp(glConst /*type*/, uint32_t size, void const * data, uint32_t /*offset*/) const
  {
    TEST_EQUAL(size, m_size, ());
    TEST_EQUAL(memcmp(m_mem, data, size), 0, ());
  }
};

class BatcherExpectations
{
public:
  BatcherExpectations()
    : m_indexBufferID(1)
    , m_dataBufferID(2)
  {
  }

  typedef function<void (Batcher *, GLState const &, RefPointer<AttributeProvider>)> TBatcherCallFn;
  void RunTest(float * vertexes, uint16_t * indexes,
               uint16_t vertexCount, uint16_t vertexComponentCount,
               uint16_t indexCount, TBatcherCallFn const & fn)
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

  void ExpectBufferCreation(uint16_t vertxeCount, uint16_t indexCount,
                            MemoryComparer const & indexCmp, MemoryComparer const & vertexCmp)
  {
    InSequence seq;

    // Index buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(m_indexBufferID));
    EXPECTGL(glBindBuffer(m_indexBufferID, gl_const::GLElementArrayBuffer));
    EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, _, NULL, _));

    // upload indexes
    EXPECTGL(glBindBuffer(m_indexBufferID, gl_const::GLElementArrayBuffer));
    EXPECTGL(glBufferSubData(gl_const::GLElementArrayBuffer, indexCount * sizeof(unsigned short), _, 0))
        .WillOnce(Invoke(&indexCmp, &MemoryComparer::cmp));

    // data buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(m_dataBufferID));
    EXPECTGL(glBindBuffer(m_dataBufferID, gl_const::GLArrayBuffer));
    EXPECTGL(glBufferData(gl_const::GLArrayBuffer, _, NULL, _));

    // upload data
    EXPECTGL(glBindBuffer(m_dataBufferID, gl_const::GLArrayBuffer));
    EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer, vertxeCount * sizeof(float), _, 0))
        .WillOnce(Invoke(&vertexCmp, &MemoryComparer::cmp));
  }

  void ExpectBufferDeletion()
  {
    InSequence seq;
    EXPECTGL(glDeleteBuffer(m_indexBufferID));
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
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, VERTEX_COUNT, bind(&Batcher::InsertTriangleList, _1, _2, _3));
}

UNIT_TEST(BatchListOfStript_4stride)
{
  int const VERTEX_COUNT = 12;
  int const INDEX_COUNT = 18;

  float data[3 * VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT * 3; ++i)
    data[i] = (float)i;

  unsigned short indexes[INDEX_COUNT] =
    { 0, 1, 2, 1, 2, 3, 4, 5, 6, 5, 6, 7, 8, 9, 10, 9, 10, 11};

  BatcherExpectations expectations;
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, bind(&Batcher::InsertListOfStrip, _1, _2, _3, 4));
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
      1, 2, 3,
      2, 3, 4,
      5, 6, 7,
      6, 7, 8,
      7, 8, 9,
      10, 11, 12,
      11, 12, 13,
      12, 13, 14 };

  BatcherExpectations expectations;
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, bind(&Batcher::InsertListOfStrip, _1, _2, _3, 5));
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
      1, 2, 3,
      2, 3, 4,
      3, 4, 5,
      6, 7, 8,
      7, 8, 9,
      8, 9, 10,
      9, 10, 11,
      12, 13, 14,
      13, 14, 15,
      14, 15, 16,
      15, 16, 17};

  BatcherExpectations expectations;
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, INDEX_COUNT, bind(&Batcher::InsertListOfStrip, _1, _2, _3, 6));
}

UNIT_TEST(BatchListOfStript_partial)
{
  uint32_t const IndexBufferID1 = 1;
  uint32_t const VertexBufferID1 = 2;
  uint32_t const IndexBufferID2 = 3;
  uint32_t const VertexBufferID2 = 4;

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
      1, 2, 3,
      4, 5, 6,
      5, 6, 7,
      8, 9, 10,
      9, 10, 11,
      0, 1, 2, // start new buffer
      1, 2, 3};

  MemoryComparer vertexCmp1(vertexData, FirstBufferVertexPortion * ComponentCount * sizeof(float));
  MemoryComparer indexCmp1(indexData, FirstBufferIndexPortion * sizeof(uint16_t));

  MemoryComparer vertexCmp2(vertexData + FirstBufferVertexPortion * ComponentCount,
                            SecondBufferVertexPortion * ComponentCount * sizeof(float));
  MemoryComparer indexCmp2(indexData + FirstBufferIndexPortion, SecondBufferIndexPortion * sizeof(uint16_t));

  InSequence seq;

  // Index buffer creation
  EXPECTGL(glGenBuffer()).WillOnce(Return(IndexBufferID1));
  EXPECTGL(glBindBuffer(IndexBufferID1, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, _, NULL, _));

  // upload indexes
  EXPECTGL(glBindBuffer(IndexBufferID1, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLElementArrayBuffer, FirstBufferIndexPortion * sizeof(unsigned short), _, 0))
      .WillOnce(Invoke(&indexCmp1, &MemoryComparer::cmp));

  // data buffer creation
  EXPECTGL(glGenBuffer()).WillOnce(Return(VertexBufferID1));
  EXPECTGL(glBindBuffer(VertexBufferID1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, _, NULL, _));

  // upload data
  EXPECTGL(glBindBuffer(VertexBufferID1, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer,
                           ComponentCount * FirstBufferVertexPortion * sizeof(float), _, 0))
          .WillOnce(Invoke(&vertexCmp1, &MemoryComparer::cmp));

  // Create second buffer on ChangeBuffer operation
  // Index buffer creation
  EXPECTGL(glGenBuffer()).WillOnce(Return(IndexBufferID2));
  EXPECTGL(glBindBuffer(IndexBufferID2, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLElementArrayBuffer, _, NULL, _));

  // upload indexes
  EXPECTGL(glBindBuffer(IndexBufferID2, gl_const::GLElementArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLElementArrayBuffer, SecondBufferIndexPortion * sizeof(unsigned short), _, 0))
      .WillOnce(Invoke(&indexCmp2, &MemoryComparer::cmp));

  // data buffer creation
  EXPECTGL(glGenBuffer()).WillOnce(Return(VertexBufferID2));
  EXPECTGL(glBindBuffer(VertexBufferID2, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferData(gl_const::GLArrayBuffer, _, NULL, _));

  // upload data
  EXPECTGL(glBindBuffer(VertexBufferID2, gl_const::GLArrayBuffer));
  EXPECTGL(glBufferSubData(gl_const::GLArrayBuffer,
                           ComponentCount * SecondBufferVertexPortion * sizeof(float), _, 0))
          .WillOnce(Invoke(&vertexCmp2, &MemoryComparer::cmp));

  EXPECTGL(glDeleteBuffer(IndexBufferID1));
  EXPECTGL(glDeleteBuffer(VertexBufferID1));
  EXPECTGL(glDeleteBuffer(IndexBufferID2));
  EXPECTGL(glDeleteBuffer(VertexBufferID2));

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
  Batcher batcher(30, 12);
  batcher.StartSession(bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
  batcher.InsertListOfStrip(state, MakeStackRefPointer(&provider), 4);
  batcher.EndSession();

  for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
    vaoAcceptor.m_vao[i].Destroy();
}
