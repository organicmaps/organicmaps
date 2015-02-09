#pragma once

#include "std/cstdint.hpp"

typedef uint32_t glConst;

namespace gl_const
{
extern const glConst GLUnpackAlignment;

/// Hardware specific params
extern const glConst GLMaxFragmentTextures;
extern const glConst GLMaxVertexTextures;
extern const glConst GLMaxTextureSize;

/// Buffer targets
extern const glConst GLArrayBuffer;
extern const glConst GLElementArrayBuffer;

/// Buffer params
extern const glConst GLBufferSize;
extern const glConst GLBufferUsage;

/// VBO Access
extern const glConst GLWriteOnly;

/// BufferUsage
extern const glConst GLStaticDraw;
extern const glConst GLStreamDraw;
extern const glConst GLDynamicDraw;

/// ShaderType
extern const glConst GLVertexShader;
extern const glConst GLFragmentShader;
extern const glConst GLCurrentProgram;

/// Texture layouts
extern const glConst GLRGBA;
extern const glConst GLRGB;
extern const glConst GLAlpha;
extern const glConst GLLuminance;
extern const glConst GLAlphaLuminance;

/// Texture layout size
extern const glConst GLRGBA8;
extern const glConst GLRGBA4;
extern const glConst GLAlpha8;
extern const glConst GLLuminance8;
extern const glConst GLAlphaLuminance8;
extern const glConst GLAlphaLuminance4;

/// Pixel type for texture upload
extern const glConst GL8BitOnChannel;
extern const glConst GL4BitOnChannel;

/// Texture targets
extern const glConst GLTexture2D;

/// Texture uniform blocks
extern const glConst GLTexture0;

/// Texture param names
extern const glConst GLMinFilter;
extern const glConst GLMagFilter;
extern const glConst GLWrapS;
extern const glConst GLWrapT;

/// Texture Wrap Modes
extern const glConst GLRepeate;
extern const glConst GLMirroredRepeate;
extern const glConst GLClampToEdge;

/// Texture Filter Modes
extern const glConst GLLinear;
extern const glConst GLNearest;

/// OpenGL types
extern const glConst GLByteType;
extern const glConst GLUnsignedByteType;
extern const glConst GLShortType;
extern const glConst GLUnsignedShortType;
extern const glConst GLIntType;
extern const glConst GLUnsignedIntType;
extern const glConst GLFloatType;

extern const glConst GLFloatVec2;
extern const glConst GLFloatVec3;
extern const glConst GLFloatVec4;

extern const glConst GLIntVec2;
extern const glConst GLIntVec3;
extern const glConst GLIntVec4;

extern const glConst GLFloatMat4;

/// Blend Functions
extern const glConst GLAddBlend;
extern const glConst GLSubstractBlend;
extern const glConst GLReverseSubstrBlend;

/// Blend Factors
extern const glConst GLZero;
extern const glConst GLOne;
extern const glConst GLSrcColor;
extern const glConst GLOneMinusSrcColor;
extern const glConst GLDstColor;
extern const glConst GLOneMinusDstColor;
extern const glConst GLSrcAlfa;
extern const glConst GLOneMinusSrcAlfa;
extern const glConst GLDstAlfa;
extern const glConst GLOneMinusDstAlfa;

/// OpenGL states
extern const glConst GLDepthTest;
extern const glConst GLBlending;
extern const glConst GLCullFace;

/// Triangle faces order
extern const glConst GLClockwise;
extern const glConst GLCounterClockwise;

/// Triangle face
extern const glConst GLFront;
extern const glConst GLBack;
extern const glConst GLFrontAndBack;

/// OpenGL depth functions
extern const glConst GLNever;
extern const glConst GLLess;
extern const glConst GLEqual;
extern const glConst GLLessOrEqual;
extern const glConst GLGreat;
extern const glConst GLNotEqual;
extern const glConst GLGreatOrEqual;
extern const glConst GLAlways;

/// Program object parameter names
extern const glConst GLActiveUniforms;

} // namespace GLConst
