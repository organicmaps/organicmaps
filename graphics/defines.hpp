#pragma once

namespace graphics
{
  static const int maxDepth = 20000;

  enum EShaderType
  {
    EVertexShader,
    EFragmentShader
  };

  enum ESemantic
  {
    ESemPosition,
    ESemNormal,
    ESemTexCoord0,
    ESemSampler0,
    ESemModelView,
    ESemProjection
  };

  void convert(char const * name, ESemantic & sem);

  enum EDataType
  {
    EInteger,
    EIntegerVec2,
    EIntegerVec3,
    EIntegerVec4,
    EFloat,
    EFloatVec2,
    EFloatVec3,
    EFloatVec4,
    EFloatMat2,
    EFloatMat3,
    EFloatMat4,
    ESampler2D
  };

  enum EPrimitives
  {
    ETriangles,
    ETrianglesFan,
    ETrianglesStrip
  };

  enum EMatrix
  {
    EModelView,
    EProjection
  };

  enum EDepthFunc
  {
    ELessEqual
  };

  enum EPosition
  {
    EPosCenter = 0x00,
    EPosAbove = 0x01,
    EPosUnder = 0x02,
    EPosLeft = 0x04,
    EPosRight = 0x10,
    EPosAboveLeft = EPosAbove | EPosLeft,
    EPosAboveRight = EPosAbove | EPosRight,
    EPosUnderLeft = EPosUnder | EPosLeft,
    EPosUnderRight = EPosUnder | EPosRight
  };
}
