#include "../../testing/testing.hpp"

#include "../gpu_program_manager.hpp"
#include "../gpu_program.hpp"
#include "../shader_def.hpp"
#include "../uniform_value.hpp"

#include "glmock_functions.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::AnyOf;
using ::testing::IgnoreResult;
using ::testing::Invoke;
using ::testing::InSequence;

namespace
{
  template<typename T>
  class MemoryComparer
  {
  public:
    MemoryComparer(const T * memory, uint32_t size)
      : m_result(false)
      , m_memory(memory)
      , m_size(size)
    {
    }

    void Compare(int32_t id, const T * memory)
    {
      m_result = memcmp(m_memory, memory, m_size) == 0;
    }

    bool GetResult() const
    {
      return m_result;
    }

  private:
    bool m_result;
    const T * m_memory;
    uint32_t m_size;
  };
}

UNIT_TEST(UniformValueTest)
{
  const uint32_t VertexShaderID = 1;
  const uint32_t FragmentShaderID = 2;
  const uint32_t ProgramID = 3;

  const int32_t positionLoc = 10;
  const int32_t modelViewLoc = 11;

  float matrix[16] =
  {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };

  MemoryComparer<float> comparer(matrix, 16 * sizeof(float));

  {
    InSequence seq;
    // vertexShader->Ref()
    EXPECTGL(glCreateShader(GLConst::GLVertexShader)).WillOnce(Return(VertexShaderID));
    EXPECTGL(glShaderSource(VertexShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(VertexShaderID, _)).WillOnce(Return(true));
    // fragmentShader->Ref()
    EXPECTGL(glCreateShader(GLConst::GLFragmentShader)).WillOnce(Return(FragmentShaderID));
    EXPECTGL(glShaderSource(FragmentShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(FragmentShaderID, _)).WillOnce(Return(true));

    EXPECTGL(glCreateProgram()).WillOnce(Return(ProgramID));
    EXPECTGL(glAttachShader(ProgramID, VertexShaderID));
    EXPECTGL(glAttachShader(ProgramID, FragmentShaderID));

    EXPECTGL(glLinkProgram(ProgramID, _)).WillOnce(Return(true));

    EXPECTGL(glDetachShader(ProgramID, VertexShaderID));
    EXPECTGL(glDetachShader(ProgramID, FragmentShaderID));

    EXPECTGL(glUseProgram(ProgramID));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3, 4));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f, 4.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "viewModel")).WillOnce(Return(modelViewLoc));
    EXPECTGL(glUniformMatrix4x4Value(modelViewLoc, _))
        .WillOnce(Invoke(&comparer, &MemoryComparer<float>::Compare));

    EXPECTGL(glUseProgram(0));
    EXPECTGL(glDeleteProgram(ProgramID));

    EXPECTGL(glDeleteShader(VertexShaderID));
    EXPECTGL(glDeleteShader(FragmentShaderID));
  }

  GpuProgramManager * manager = new GpuProgramManager();
  RefPointer<GpuProgram> program = manager->GetProgram(gpu::SOLID_AREA_PROGRAM);

  program->Bind();

  {
    UniformValue v("position", 1);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1, 2);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1, 2, 3);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1, 2, 3, 4);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1.0f, 2.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1.0f, 2.0f, 3.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position", 1.0f, 2.0f, 3.0f, 4.0f);
    v.Apply(program);
  }

  {
    UniformValue v("viewModel", matrix);
    v.Apply(program);
  }

  program = RefPointer<GpuProgram>();
  delete manager;
}
