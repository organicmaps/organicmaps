#pragma once

#include <cstdint>

using glConst = uint32_t;

namespace gl_const
{
extern glConst const GLUnpackAlignment;

extern glConst const GLRenderer;
extern glConst const GLVendor;
extern glConst const GLVersion;

extern glConst const glContextFlags;
extern glConst const glContextFlagDebugBit;

/// Clear bits
extern glConst const GLColorBit;
extern glConst const GLDepthBit;
extern glConst const GLStencilBit;

/// Hardware specific params
extern glConst const GLMaxFragmentTextures;
extern glConst const GLMaxVertexTextures;
extern glConst const GLMaxTextureSize;

/// Buffer targets
extern glConst const GLArrayBuffer;
extern glConst const GLElementArrayBuffer;
extern glConst const GLPixelBufferWrite;

/// Buffer params
extern glConst const GLBufferSize;
extern glConst const GLBufferUsage;

/// VBO Access
extern glConst const GLWriteOnly;
extern glConst const GLReadOnly;

/// MapBufferRange
extern glConst const GLWriteBufferBit;
extern glConst const GLReadBufferBit;
extern glConst const GLInvalidateRange;
extern glConst const GLInvalidateBuffer;
extern glConst const GLFlushExplicit;
extern glConst const GLUnsynchronized;

/// BufferUsage
extern glConst const GLStaticDraw;
extern glConst const GLStreamDraw;
extern glConst const GLDynamicDraw;

/// ShaderType
extern glConst const GLVertexShader;
extern glConst const GLFragmentShader;
extern glConst const GLCurrentProgram;

/// Texture layouts
extern glConst const GLRGBA;
extern glConst const GLRGB;
extern glConst const GLAlpha;
extern glConst const GLLuminance;
extern glConst const GLAlphaLuminance;
extern glConst const GLDepthComponent;
extern glConst const GLDepthStencil;

/// Texture layout size
extern glConst const GLRGBA8;
extern glConst const GLRGBA4;
extern glConst const GLAlpha8;
extern glConst const GLLuminance8;
extern glConst const GLAlphaLuminance8;
extern glConst const GLAlphaLuminance4;
extern glConst const GLRed;
extern glConst const GLRedGreen;

/// Pixel type for texture upload
extern glConst const GL8BitOnChannel;
extern glConst const GL4BitOnChannel;

/// Texture targets
extern glConst const GLTexture2D;

/// Texture uniform blocks
extern glConst const GLTexture0;

/// Texture param names
extern glConst const GLMinFilter;
extern glConst const GLMagFilter;
extern glConst const GLWrapS;
extern glConst const GLWrapT;

/// Texture Wrap Modes
extern glConst const GLRepeat;
extern glConst const GLMirroredRepeat;
extern glConst const GLClampToEdge;

/// Texture Filter Modes
extern glConst const GLLinear;
extern glConst const GLNearest;

/// OpenGL types
extern glConst const GLByteType;
extern glConst const GLUnsignedByteType;
extern glConst const GLShortType;
extern glConst const GLUnsignedShortType;
extern glConst const GLIntType;
extern glConst const GLUnsignedIntType;
extern glConst const GLFloatType;
extern glConst const GLUnsignedInt24_8Type;

extern glConst const GLFloatVec2;
extern glConst const GLFloatVec3;
extern glConst const GLFloatVec4;

extern glConst const GLIntVec2;
extern glConst const GLIntVec3;
extern glConst const GLIntVec4;

extern glConst const GLFloatMat4;

extern glConst const GLSampler2D;

/// Blend Functions
extern glConst const GLAddBlend;
extern glConst const GLSubstractBlend;
extern glConst const GLReverseSubstrBlend;

/// Blend Factors
extern glConst const GLZero;
extern glConst const GLOne;
extern glConst const GLSrcColor;
extern glConst const GLOneMinusSrcColor;
extern glConst const GLDstColor;
extern glConst const GLOneMinusDstColor;
extern glConst const GLSrcAlpha;
extern glConst const GLOneMinusSrcAlpha;
extern glConst const GLDstAlpha;
extern glConst const GLOneMinusDstAlpha;

/// OpenGL states
extern glConst const GLDepthTest;
extern glConst const GLBlending;
extern glConst const GLCullFace;
extern glConst const GLScissorTest;
extern glConst const GLStencilTest;
extern glConst const GLDebugOutput;
extern glConst const GLDebugOutputSynchronous;

extern glConst const GLDontCare;
extern glConst const GLDontCare;
extern glConst const GLTrue;
extern glConst const GLFalse;

// OpenGL source type
extern glConst const GLDebugSourceApi;
extern glConst const GLDebugSourceShaderCompiler;
extern glConst const GLDebugSourceThirdParty;
extern glConst const GLDebugSourceApplication;
extern glConst const GLDebugSourceOther;

// OpenGL debug type
extern glConst const GLDebugTypeError;
extern glConst const GLDebugDeprecatedBehavior;
extern glConst const GLDebugUndefinedBehavior;
extern glConst const GLDebugPortability;
extern glConst const GLDebugPerformance;
extern glConst const GLDebugOther;

// OpenGL debug severity
extern glConst const GLDebugSeverityLow;
extern glConst const GLDebugSeverityMedium;
extern glConst const GLDebugSeverityHigh;
extern glConst const GLDebugSeverityNotification;

/// Triangle faces order
extern glConst const GLClockwise;
extern glConst const GLCounterClockwise;

/// Triangle face
extern glConst const GLFront;
extern glConst const GLBack;
extern glConst const GLFrontAndBack;

/// OpenGL depth functions
extern glConst const GLNever;
extern glConst const GLLess;
extern glConst const GLEqual;
extern glConst const GLLessOrEqual;
extern glConst const GLGreat;
extern glConst const GLNotEqual;
extern glConst const GLGreatOrEqual;
extern glConst const GLAlways;

/// OpenGL stencil functions
extern glConst const GLKeep;
extern glConst const GLIncr;
extern glConst const GLDecr;
extern glConst const GLInvert;
extern glConst const GLReplace;
extern glConst const GLIncrWrap;
extern glConst const GLDecrWrap;

/// Program object parameter names
extern glConst const GLActiveUniforms;

/// Draw primitives
extern glConst const GLLines;
extern glConst const GLLineStrip;
extern glConst const GLTriangles;
extern glConst const GLTriangleStrip;

/// Framebuffer attachment points
extern glConst const GLColorAttachment;
extern glConst const GLDepthAttachment;
extern glConst const GLStencilAttachment;
extern glConst const GLDepthStencilAttachment;

/// Framebuffer status
extern glConst const GLFramebufferComplete;
}  // namespace gl_const
