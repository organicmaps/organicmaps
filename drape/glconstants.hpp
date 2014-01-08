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

  // Program object parameter names
  extern const glConst GLActiveUniforms;
}
