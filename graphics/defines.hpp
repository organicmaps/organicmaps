#pragma once

namespace graphics
{
  static const int maxDepth = 20000;

  enum EShaderType
  {
    EVertexShader,
    EFragmentShader
  };

  enum EDataType
  {
    EFloat,
    EUnsigned,
    EInteger
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
