#include "testing/testing.hpp"

#include "drape/gpu_program.hpp"
#include "drape/gpu_program_info.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_value.hpp"

#include "drape/drape_tests/glmock_functions.hpp"

#include <cstring>
#include <string>
#include <utility>

#include <gmock/gmock.h>
#include <drape/drape_global.hpp>

using ::testing::_;
using ::testing::Return;
using ::testing::AnyOf;
using ::testing::IgnoreResult;
using ::testing::Invoke;
using ::testing::InSequence;
using namespace dp;
using namespace std;

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

class TestShaderMapper : public gpu::GpuProgramGetter
{
public:
  TestShaderMapper()
  {
    m_vertexShader = "void main() { gl_Position = vec4(0.0, 0.0, 0.0, 1.0); }";
    m_fragmentShader = "void main() { gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0); }";
    m_info.m_vertexIndex = 0;
    m_info.m_fragmentIndex = 1;
    m_info.m_textureSlotsCount = 0;
    m_info.m_vertexSource = m_vertexShader.c_str();
    m_info.m_fragmentSource = m_fragmentShader.c_str();
  }
  gpu::GpuProgramInfo const & GetProgramInfo(int program) const override
  {
    return m_info;
  }
private:
  gpu::GpuProgramInfo m_info;
  std::string m_vertexShader;
  std::string m_fragmentShader;
};
} // namespace

UNIT_TEST(UniformValueTest)
{
  uint32_t const VertexShaderID = 1;
  uint32_t const FragmentShaderID = 2;
  uint32_t const ProgramID = 2;

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
    EXPECTGL(glGetInteger(gl_const::GLMaxVertexTextures)).Times(1);
    // vertexShader->Ref()
    EXPECTGL(glCreateShader(gl_const::GLVertexShader)).WillOnce(Return(VertexShaderID));
    EXPECTGL(glShaderSource(VertexShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(VertexShaderID, _)).WillOnce(Return(true));
    // fragmentShader->Ref()
    EXPECTGL(glCreateShader(gl_const::GLFragmentShader)).WillOnce(Return(FragmentShaderID));
    EXPECTGL(glShaderSource(FragmentShaderID, _)).Times(1);
    EXPECTGL(glCompileShader(FragmentShaderID, _)).WillOnce(Return(true));

    EXPECTGL(glCreateProgram()).WillOnce(Return(ProgramID));
    EXPECTGL(glAttachShader(ProgramID, VertexShaderID));
    EXPECTGL(glAttachShader(ProgramID, FragmentShaderID));

    EXPECTGL(glLinkProgram(ProgramID, _)).WillOnce(Return(true));

    EXPECTGL(glGetProgramiv(ProgramID, gl_const::GLActiveUniforms)).WillOnce(Return(9));
    for (int i = 0; i < 9; i++)
    {
      EXPECTGL(glGetActiveUniform(ProgramID, _, _, _, _)).WillOnce(Invoke(mock_glGetActiveUniform));
      EXPECTGL(glGetUniformLocation(ProgramID, _)).WillOnce(Return(i == 8 ? modelViewLoc : positionLoc));
    }

    EXPECTGL(glDetachShader(ProgramID, VertexShaderID));
    EXPECTGL(glDetachShader(ProgramID, FragmentShaderID));

    EXPECTGL(glUseProgram(ProgramID));

    EXPECTGL(glUniformValuei(positionLoc, 1));

    EXPECTGL(glUniformValuei(positionLoc, 1, 2));

    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3));

    EXPECTGL(glUniformValuei(positionLoc, 1, 2, 3, 4));

    EXPECTGL(glUniformValuef(positionLoc, 1.0f));

    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f));

    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f));

    EXPECTGL(glUniformValuef(positionLoc, 1.0f, 2.0f, 3.0f, 4.0f));

    EXPECTGL(glUniformMatrix4x4Value(modelViewLoc, _))
        .WillOnce(Invoke(&comparer, &MemoryComparer<float>::Compare));

    EXPECTGL(glUseProgram(0));
    EXPECTGL(glDeleteProgram(ProgramID));
    EXPECTGL(glDeleteShader(AnyOf(VertexShaderID, FragmentShaderID))).Times(2);
  }

  drape_ptr<GpuProgramManager> manager = make_unique_dp<GpuProgramManager>();
  manager->Init(make_unique_dp<TestShaderMapper>());
  ref_ptr<GpuProgram> program = manager->GetProgram(0);

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
