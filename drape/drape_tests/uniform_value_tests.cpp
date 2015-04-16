#include "testing/testing.hpp"

#include "drape/gpu_program_manager.hpp"
#include "drape/gpu_program.hpp"
#include "drape/shader_def.hpp"
#include "drape/uniform_value.hpp"

#include "drape/drape_tests/glmock_functions.hpp"

#include "std/cstring.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::AnyOf;
using ::testing::IgnoreResult;
using ::testing::Invoke;
using ::testing::InSequence;
using namespace dp;

namespace
{

template<typename T>
class MemoryComparer
{
public:
  MemoryComparer(T const * memory, uint32_t size)
    : m_result(false)
    , m_memory(memory)
    , m_size(size)
  {
  }

  void Compare(int32_t id, T const * memory)
  {
    m_result = memcmp(m_memory, memory, m_size) == 0;
  }

  bool GetResult() const
  {
    return m_result;
  }

private:
  bool m_result;
  T const * m_memory;
  uint32_t m_size;
};

#ifdef DEBUG
void mock_glGetActiveUniform(uint32_t programID,
                             uint32_t index,
                             int32_t * size,
                             glConst * type,
                             string & name)
{
  *size = 1;
  if (index < 9)
  {
    static pair<string, glConst> mockUniforms[9] =
    {
      make_pair("position0", gl_const::GLIntType),
      make_pair("position1", gl_const::GLIntVec2),
      make_pair("position2", gl_const::GLIntVec3),
      make_pair("position3", gl_const::GLIntVec4),
      make_pair("position4", gl_const::GLFloatType),
      make_pair("position5", gl_const::GLFloatVec2),
      make_pair("position6", gl_const::GLFloatVec3),
      make_pair("position7", gl_const::GLFloatVec4),
      make_pair("viewModel", gl_const::GLFloatMat4)
    };
    name = mockUniforms[index].first;
    *type = mockUniforms[index].second;
  }
  else
    ASSERT(false, ("Undefined index:", index));
}
#endif

} // namespace

UNIT_TEST(UniformValueTest)
{
  uint32_t const VertexShaderID = 1;
  uint32_t const FragmentShaderID = 2;
  uint32_t const ProgramID = 3;

  int32_t const positionLoc = 10;
  int32_t const modelViewLoc = 11;


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
    EXPECTGL(glCreateShader(gl_const::GLVertexShader)).WillOnce(Return(VertexShaderID));
    EXPECTGL(glShaderSource(VertexShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(VertexShaderID, _)).WillOnce(Return(true));
    // fragmentShader->Ref()
    EXPECTGL(glCreateShader(gl_const::GLFragmentShader)).WillOnce(Return(FragmentShaderID));
    //EXPECTGL(glGetInteger(gl_const::GLMaxFragmentTextures)).WillOnce(Return(8));
    EXPECTGL(glShaderSource(FragmentShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(FragmentShaderID, _)).WillOnce(Return(true));

    EXPECTGL(glCreateProgram()).WillOnce(Return(ProgramID));
    EXPECTGL(glAttachShader(ProgramID, VertexShaderID));
    EXPECTGL(glAttachShader(ProgramID, FragmentShaderID));

    EXPECTGL(glLinkProgram(ProgramID, _)).WillOnce(Return(true));

    EXPECTGL(glDetachShader(ProgramID, VertexShaderID));
    EXPECTGL(glDetachShader(ProgramID, FragmentShaderID));

#ifdef DEBUG
    EXPECTGL(glGetProgramiv(ProgramID, gl_const::GLActiveUniforms)).WillOnce(Return(9));
    EXPECTGL(glGetActiveUniform(ProgramID, _, _, _, _)).Times(9).WillRepeatedly(Invoke(mock_glGetActiveUniform));
#endif

    EXPECTGL(glUseProgram(ProgramID));

    EXPECTGL(glGetUniformLocation(ProgramID, "position0")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1));

    EXPECTGL(glGetUniformLocation(ProgramID, "position1")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2));

    EXPECTGL(glGetUniformLocation(ProgramID, "position2")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3));

    EXPECTGL(glGetUniformLocation(ProgramID, "position3")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3, 4));

    EXPECTGL(glGetUniformLocation(ProgramID, "position4")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position5")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position6")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "position7")).WillOnce(Return(positionLoc));
    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f, 4.0f));

    EXPECTGL(glGetUniformLocation(ProgramID, "viewModel")).WillOnce(Return(modelViewLoc));
    EXPECTGL(glUniformMatrix4x4Value(modelViewLoc, _))
        .WillOnce(Invoke(&comparer, &MemoryComparer<float>::Compare));

    EXPECTGL(glUseProgram(0));
    EXPECTGL(glDeleteProgram(ProgramID));
    EXPECTGL(glDeleteShader(AnyOf(VertexShaderID, FragmentShaderID))).Times(2);
  }

  drape_ptr<GpuProgramManager> manager = make_unique_dp<GpuProgramManager>();
  ref_ptr<GpuProgram> program = manager->GetProgram(gpu::TEXTURING_PROGRAM);

  program->Bind();

  {
    UniformValue v("position0", 1);
    v.Apply(program);
  }

  {
    UniformValue v("position1", 1, 2);
    v.Apply(program);
  }

  {
    UniformValue v("position2", 1, 2, 3);
    v.Apply(program);
  }

  {
    UniformValue v("position3", 1, 2, 3, 4);
    v.Apply(program);
  }

  {
    UniformValue v("position4", 1.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position5", 1.0f, 2.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position6", 1.0f, 2.0f, 3.0f);
    v.Apply(program);
  }

  {
    UniformValue v("position7", 1.0f, 2.0f, 3.0f, 4.0f);
    v.Apply(program);
  }

  {
    UniformValue v("viewModel", matrix);
    v.Apply(program);
  }
}
