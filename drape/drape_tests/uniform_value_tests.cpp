#include "../../testing/testing.hpp"

#include "../gpu_program.hpp"
#include "../uniform_value.hpp"

#include "glmock_functions.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::AnyOf;
using ::testing::IgnoreResult;
using ::testing::Invoke;

namespace
{
  template<typename T>
  class MemoryComparer
  {
  public:
    MemoryComparer(T * memory, uint32_t size)
      : m_result(false)
      , m_memory(memory)
      , m_size(size)
    {
    }

    void Compare(int32_t id, T * memory)
    {
      m_result = memcmp(m_memory, memory, m_size) == 0;
    }

    bool GetResult() const
    {
      return m_result;
    }

  private:
    bool m_result;
    T * m_memory;
    uint32_t m_size;
  };
}

UNIT_TEST(UniformValueTest)
{
  ShaderReference vertexShader("", ShaderReference::VertexShader);
  ShaderReference fragmentShader("", ShaderReference::FragmentShader);

  // vertexShader->Ref()
  EXPECTGL(glCreateShader(GLConst::GLVertexShader)).WillOnce(Return(1));
  EXPECTGL(glShaderSource(1, "")).Times(1);
  EXPECTGL(glCompileShader(1, _)).WillOnce(Return(true));
  // fragmentShader->Ref()
  EXPECTGL(glCreateShader(GLConst::GLFragmentShader)).WillOnce(Return(2));
  EXPECTGL(glShaderSource(2, "")).Times(1);
  EXPECTGL(glCompileShader(2, _)).WillOnce(Return(true));

  EXPECTGL(glCreateProgram()).WillOnce(Return(3));
  EXPECTGL(glAttachShader(3, AnyOf(1, 2))).Times(2);
  EXPECTGL(glLinkProgram(3, _)).WillOnce(Return(true));
  EXPECTGL(glDetachShader(3, AnyOf(1, 2))).Times(2);
  EXPECTGL(glDeleteShader(AnyOf(1, 2))).Times(2);

  GpuProgram * program = new GpuProgram(ReferencePoiner<ShaderReference>(&vertexShader),
                                        ReferencePoiner<ShaderReference>(&fragmentShader));

  EXPECTGL(glUseProgram(3)).Times(1);
  program->Bind();

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuei(3, 1)).Times(1);
    UniformValue v("position", 1);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuei(3, 1, 2)).Times(1);
    UniformValue v("position", 1, 2);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuei(3, 1, 2, 3)).Times(1);
    UniformValue v("position", 1, 2, 3);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuei(3, 1, 2, 3, 4)).Times(1);
    UniformValue v("position", 1, 2, 3, 4);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuef(3, 1.0f)).Times(1);
    UniformValue v("position", 1.0f);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuef(3, 1.0f, 2.0f)).Times(1);
    UniformValue v("position", 1.0f, 2.0f);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuef(3, 1.0f, 2.0f, 3.0f)).Times(1);
    UniformValue v("position", 1.0f, 2.0f, 3.0f);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    EXPECTGL(glGetUniformLocation(3, "position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuef(3, 1.0f, 2.0f, 3.0f, 4.0f)).Times(1);
    UniformValue v("position", 1.0f, 2.0f, 3.0f, 4.0f);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  {
    float matrix[16] =
    {
      1.0f, 0.0f, 0.0f, 0.0f,
      0.0f, 1.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 1.0f, 0.0f,
      0.0f, 0.0f, 0.0f, 1.0f
    };

    MemoryComparer<float> comparer(matrix, 16 * sizeof(float));

    EXPECTGL(glGetUniformLocation(3, "viewModel")).WillOnce(Return(4));
    EXPECTGL(glUniformMatrix4x4Value(4, _))
        .WillOnce(Invoke(&comparer, &MemoryComparer<float>::Compare));
    UniformValue v("viewModel", matrix);
    v.Apply(ReferencePoiner<GpuProgram>(program));
  }

  EXPECTGL(glUseProgram(0)).Times(1);
  EXPECTGL(glDeleteProgram(3)).Times(1);

  delete program;
}
