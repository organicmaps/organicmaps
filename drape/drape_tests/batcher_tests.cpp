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

#include <gmock/gmock.h>

using testing::_;
using testing::Return;
using testing::InSequence;
using testing::Invoke;
using testing::IgnoreResult;
using testing::AnyOf;

namespace
{
  struct VAOAcceptor
  {
    virtual void FlushFullBucket(const GLState & state, TransferPointer<RenderBucket> bucket)
    {
      MasterPointer<RenderBucket> masterBucket(bucket);
      masterBucket->GetBuffer()->Build(m_program);
      m_vao.push_back(masterBucket);
    }

    vector<MasterPointer<RenderBucket> > m_vao;
    RefPointer<GpuProgram> m_program;
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

    void cmp(glConst /*type*/, uint32_t size, void const * data, uint32_t /*offset*/)
    {
      TEST_EQUAL(size, m_size, ());
      TEST_EQUAL(memcmp(m_mem, data, size), 0, ());
    }
  };
}

UNIT_TEST(BatchLists_Test)
{
  const uint32_t VertexShaderID = 10;
  const uint32_t FragmentShaderID = 11;
  const uint32_t ProgramID = 20;

  const uint32_t IndexBufferID = 1;
  const uint32_t DataBufferID = 2;
  const uint32_t VaoID = 3;

  const int VERTEX_COUNT = 12;
  float data[3 * VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT * 3; ++i)
    data[i] = (float)i;

  unsigned short indexes[VERTEX_COUNT];
  for (int i = 0; i < VERTEX_COUNT; ++i)
    indexes[i] = i;

  MemoryComparer dataCmp(data, 3 * VERTEX_COUNT * sizeof(float));
  MemoryComparer indexCmp(indexes, VERTEX_COUNT * sizeof(unsigned short));

  {
    InSequence shaderSeq;
    EXPECTGL(glCreateShader(GLConst::GLVertexShader)).WillOnce(Return(VertexShaderID));
    EXPECTGL(glShaderSource(VertexShaderID, _));
    EXPECTGL(glCompileShader(VertexShaderID, _)).WillOnce(Return(true));

    EXPECTGL(glCreateShader(GLConst::GLFragmentShader)).WillOnce(Return(FragmentShaderID));
    EXPECTGL(glShaderSource(FragmentShaderID, _));
    EXPECTGL(glCompileShader(FragmentShaderID, _)).WillOnce(Return(true));

    EXPECTGL(glCreateProgram()).WillOnce(Return(ProgramID));
    EXPECTGL(glAttachShader(ProgramID, VertexShaderID));
    EXPECTGL(glAttachShader(ProgramID, FragmentShaderID));
    EXPECTGL(glLinkProgram(ProgramID, _)).WillOnce(Return(true));
    EXPECTGL(glDetachShader(ProgramID, VertexShaderID));
    EXPECTGL(glDetachShader(ProgramID, FragmentShaderID));

#ifdef DEBUG
    EXPECTGL(glGetProgramiv(_, _));
#endif

    EXPECTGL(glDeleteShader(AnyOf(10, 11))).Times(2);
  }

  scoped_ptr<GpuProgramManager> pm(new GpuProgramManager());
  Batcher batcher;

  {
    InSequence vaoSeq;
    // Index buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(IndexBufferID));
    EXPECTGL(glBindBuffer(IndexBufferID, GLConst::GLElementArrayBuffer));
    EXPECTGL(glBufferData(GLConst::GLElementArrayBuffer, _, NULL, _));

    // upload indexes
    EXPECTGL(glBindBuffer(IndexBufferID, GLConst::GLElementArrayBuffer));
    EXPECTGL(glBufferSubData(GLConst::GLElementArrayBuffer, VERTEX_COUNT * sizeof(unsigned short), _, 0))
        .WillOnce(Invoke(&indexCmp, &MemoryComparer::cmp));

    // data buffer creation
    EXPECTGL(glGenBuffer()).WillOnce(Return(DataBufferID));
    EXPECTGL(glBindBuffer(DataBufferID, GLConst::GLArrayBuffer));
    EXPECTGL(glBufferData(GLConst::GLArrayBuffer, _, NULL, _));

    // upload data
    EXPECTGL(glBindBuffer(DataBufferID, GLConst::GLArrayBuffer));
    EXPECTGL(glBufferSubData(GLConst::GLArrayBuffer, 3 * VERTEX_COUNT * sizeof(float), _, 0))
        .WillOnce(Invoke(&dataCmp, &MemoryComparer::cmp));

    // build VertexArrayBuffer
    EXPECTGL(glHasExtension(_)).WillOnce(Return(true));
    EXPECTGL(glHasExtension(_)).WillOnce(Return(true));
    EXPECTGL(glHasExtension(_)).WillOnce(Return(true));
    EXPECTGL(glGenVertexArray()).WillOnce(Return(VaoID));
    EXPECTGL(glBindVertexArray(VaoID));

    // bind buffer pointer to program attribute
    EXPECTGL(glBindBuffer(DataBufferID, GLConst::GLArrayBuffer));
    EXPECTGL(glGetAttribLocation(ProgramID, "position")).WillOnce(Return(1));
    EXPECTGL(glEnableVertexAttribute(1));
    EXPECTGL(glVertexAttributePointer(1, 3, GLConst::GLFloatType, false, 0, 0));

    // bind index buffer
    EXPECTGL(glBindBuffer(IndexBufferID, GLConst::GLElementArrayBuffer));

    // delete bucket
    EXPECTGL(glDeleteBuffer(IndexBufferID));
    EXPECTGL(glDeleteBuffer(DataBufferID));
    EXPECTGL(glDeleteVertexArray(3));
    EXPECTGL(glUseProgram(0));
    EXPECTGL(glDeleteProgram(ProgramID));
  }

  GLState state(0, GLState::GeometryLayer);

  BindingInfo binding(1);
  BindingDecl & decl = binding.GetBindingDecl(0);
  decl.m_attributeName = "position";
  decl.m_componentCount = 3;
  decl.m_componentType = GLConst::GLFloatType;
  decl.m_offset = 0;
  decl.m_stride = 0;

  AttributeProvider provider(1, VERTEX_COUNT);
  provider.InitStream(0, binding, MakeStackRefPointer(data));

  VAOAcceptor vaoAcceptor;
  vaoAcceptor.m_program = pm->GetProgram(gpu::SOLID_AREA_PROGRAM);
  batcher.StartSession(bind(&VAOAcceptor::FlushFullBucket, &vaoAcceptor, _1, _2));
  batcher.InsertTriangleList(state, MakeStackRefPointer(&provider));
  batcher.EndSession();

  for (size_t i = 0; i < vaoAcceptor.m_vao.size(); ++i)
    vaoAcceptor.m_vao[i].Destroy();
}
