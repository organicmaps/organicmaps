#include "../vertex_array_buffer.hpp"

#include "../../base/SRC_FIRST.hpp"
#include "../../testing/testing.hpp"

#include "../../drape/object_pool.hpp"
#include "../../drape/batcher.hpp"
#include "../../drape_frontend/read_mwm_task.hpp"

#include "../../std/bind.hpp"

#include <gmock/gmock.h>
#include "glmock_functions.hpp"

using testing::_;
using testing::Return;
using testing::InSequence;
using testing::Invoke;
using testing::IgnoreResult;
using testing::AnyOf;
using namespace dp;
using namespace df;

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
               uint16_t indexCount, TBatcherCallFn const & fn, Batcher * batcher)
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
    batcher->StartSession(bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
    fn(batcher, state, MakeStackRefPointer(&provider));
    batcher->EndSession();

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

void BatchLists_Test(Batcher * batcher)
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
  expectations.RunTest(data, indexes, VERTEX_COUNT, 3, VERTEX_COUNT, bind(&Batcher::InsertTriangleList, _1, _2, _3), batcher);
}


UNIT_TEST(ObjectPoolBatcher)
{
  BatcherFactory f1;
  ObjectPool<Batcher, BatcherFactory> pool(1, f1);
  Batcher *pt1, *pt2, *pt3;
  pt1 = pool.Get();

  BatchLists_Test(pt1);

  pool.Return(pt1);
  pt2 = pool.Get();
  BatchLists_Test(pt2);
  ASSERT_EQUAL(pt1, pt2, ());
  pt3 = pool.Get();
  BatchLists_Test(pt3);

  pool.Return(pt2);
  pool.Return(pt3);
}
