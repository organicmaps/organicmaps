#pragma once

#include "../std/stdint.hpp"

typedef uint32_t glConst;

namespace GLConst
{
  /// Buffer targets
  extern const glConst GLArrayBuffer;
  extern const glConst GLElementArrayBuffer;

  /// BufferUsage
  extern const glConst GLStaticDraw;
  extern const glConst GLStreamDraw;
  extern const glConst GLDynamicDraw;

  /// ShaderType
  extern const glConst GLVertexShader;
  extern const glConst GLFragmentShader;
  extern const glConst GLCurrentProgram;

  /// Pixel type for texture upload
  extern const glConst GL8BitOnChannel;
  extern const glConst GL4BitOnChannel;

  /// OpenGL types
  extern const glConst GLByteType;
  extern const glConst GLUnsignedByteType;
  extern const glConst GLShortType;
  extern const glConst GLUnsignedShortType;
  extern const glConst GLIntType;
  extern const glConst GLUnsignedIntType;
  extern const glConst GLFloatType;
  extern const glConst GLDoubleType;

  extern const glConst GLFloatVec2;
  extern const glConst GLFloatVec3;
  extern const glConst GLFloatVec4;

  extern const glConst GLIntVec2;
  extern const glConst GLIntVec3;
  extern const glConst GLIntVec4;

  extern const glConst GLFloatMat4;


  /// OpenGL states
  extern const glConst GLDepthTest;

  /// OpenGL depth functions
  extern const glConst GLNever;
  extern const glConst GLLess;
  extern const glConst GLEqual;
  extern const glConst GLLessOrEqual;
  extern const glConst GLGreat;
  extern const glConst GLNotEqual;
  extern const glConst GLGreatOrEqual;
  extern const glConst GLAlways;

  // Program object parameter names
  extern const glConst GLActiveUniforms;
}
