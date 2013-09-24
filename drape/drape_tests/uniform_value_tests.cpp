#include "../../testing/testing.hpp"

#include "../gpu_program.hpp"
#include "../uniform_value.hpp"

#include "glmock_functions.hpp"

#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::AnyOf;

namespace
{
  class ShaderReferenceMock : public ShaderReference
  {
  public:
    ShaderReferenceMock(const string & shaderSource, Type type)
      : ShaderReference(shaderSource, type) {}

    MOCK_CONST_METHOD0(GetID, int());
    MOCK_METHOD0(Ref, void());
    MOCK_METHOD0(Deref, void());
  };

  class GpuProgramMock : public GpuProgram
  {
  public:
    GpuProgramMock()
      : GpuProgram(ReferencePoiner<ShaderReference>(NULL),
                   ReferencePoiner<ShaderReference>(NULL))
    {
    }

    GpuProgramMock(ReferencePoiner<ShaderReference> vertexShader,
                   ReferencePoiner<ShaderReference> fragmentShader)
      : GpuProgram(vertexShader, fragmentShader) {}

    MOCK_METHOD0(Bind, void());
    MOCK_METHOD0(Unbind, void());

    MOCK_CONST_METHOD1(GetUniformLocation, int8_t(const string & uniformName));
  };
}

UNIT_TEST(UniformValueTest)
{
  ShaderReferenceMock s1("", ShaderReference::VertexShader);
  ShaderReferenceMock s2("", ShaderReference::FragmentShader);

  EXPECT_CALL(s1, Ref()).Times(1);
  EXPECT_CALL(s2, Ref()).Times(1);
  EXPECT_CALL(s1, GetID()).WillRepeatedly(Return(3));
  EXPECT_CALL(s2, GetID()).WillRepeatedly(Return(4));
  EXPECTGL(glCreateProgram()).WillOnce(Return(1));
  EXPECTGL(glAttachShader(1, AnyOf(3, 4))).Times(2);
  EXPECTGL(glLinkProgram(1, _)).Times(1);
  EXPECTGL(glDetachShader(1, AnyOf(3, 4))).Times(2);
  EXPECT_CALL(s1, Deref()).Times(1);
  EXPECT_CALL(s2, Deref()).Times(1);

  GpuProgramMock * mock = new GpuProgramMock(ReferencePoiner<ShaderReference>(&s1),
                                             ReferencePoiner<ShaderReference>(&s2));

  {
    EXPECT_CALL(*mock, GetUniformLocation("position")).WillOnce(Return(3));
    EXPECTGL(glUniformValuei(3, 1)).Times(1);
    UniformValue v("position", 1);
    v.Apply(ReferencePoiner<GpuProgram>((GpuProgram *)mock));
  }
}
