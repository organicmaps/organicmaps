#pragma once

#include <cstdint>

using glConst = uint32_t;

namespace gl_const
{
extern const glConst GLUnpackAlignment;

extern const glConst GLRenderer;
extern const glConst GLVendor;
extern const glConst GLVersion;

extern const glConst glContextFlags;
extern const glConst glContextFlagDebugBit;

/// Clear bits
extern const glConst GLColorBit;
extern const glConst GLDepthBit;
extern const glConst GLStencilBit;

/// Hardware specific params
extern const glConst GLMaxFragmentTextures;
extern const glConst GLMaxVertexTextures;
extern const glConst GLMaxTextureSize;

/// Buffer targets
extern const glConst GLArrayBuffer;
extern const glConst GLElementArrayBuffer;
extern const glConst GLPixelBufferWrite;

/// Buffer params
extern const glConst GLBufferSize;
extern const glConst GLBufferUsage;

/// VBO Access
extern const glConst GLWriteOnly;
extern const glConst GLReadOnly;

/// MapBufferRange
extern const glConst GLWriteBufferBit;
extern const glConst GLReadBufferBit;
extern const glConst GLInvalidateRange;
extern const glConst GLInvalidateBuffer;
extern const glConst GLFlushExplicit;
extern const glConst GLUnsynchronized;

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
extern const glConst GLDepthComponent;
extern const glConst GLDepthStencil;

/// Texture layout size
extern const glConst GLRGBA8;
extern const glConst GLRGBA4;
extern const glConst GLAlpha8;
extern const glConst GLLuminance8;
extern const glConst GLAlphaLuminance8;
extern const glConst GLAlphaLuminance4;
extern const glConst GLRed;
extern const glConst GLRedGreen;

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
extern const glConst GLRepeat;
extern const glConst GLMirroredRepeat;
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
extern const glConst GLUnsignedInt24_8Type;

extern const glConst GLFloatVec2;
extern const glConst GLFloatVec3;
extern const glConst GLFloatVec4;

extern const glConst GLIntVec2;
extern const glConst GLIntVec3;
extern const glConst GLIntVec4;

extern const glConst GLFloatMat4;

extern const glConst GLSampler2D;

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
extern const glConst GLSrcAlpha;
extern const glConst GLOneMinusSrcAlpha;
extern const glConst GLDstAlpha;
extern const glConst GLOneMinusDstAlpha;

/// OpenGL states
extern const glConst GLDepthTest;
extern const glConst GLBlending;
extern const glConst GLCullFace;
extern const glConst GLScissorTest;
extern const glConst GLStencilTest;
extern const glConst GLDebugOutput;
extern const glConst GLDebugOutputSynchronous;

extern const glConst GLDontCare;
extern const glConst GLDontCare;
extern const glConst GLTrue;
extern const glConst GLFalse;

// OpenGL source type
extern const glConst GLDebugSourceApi;
extern const glConst GLDebugSourceShaderCompiler;
extern const glConst GLDebugSourceThirdParty;
extern const glConst GLDebugSourceApplication;
extern const glConst GLDebugSourceOther;

// OpenGL debug type
extern const glConst GLDebugTypeError;
extern const glConst GLDebugDeprecatedBehavior;
extern const glConst GLDebugUndefinedBehavior;
extern const glConst GLDebugPortability;
extern const glConst GLDebugPerformance;
extern const glConst GLDebugOther;

// OpenGL debug severity
extern const glConst GLDebugSeverityLow;
extern const glConst GLDebugSeverityMedium;
extern const glConst GLDebugSeverityHigh;
extern const glConst GLDebugSeverityNotification;

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

/// OpenGL stencil functions
extern const glConst GLKeep;
extern const glConst GLIncr;
extern const glConst GLDecr;
extern const glConst GLInvert;
extern const glConst GLReplace;
extern const glConst GLIncrWrap;
extern const glConst GLDecrWrap;

/// Program object parameter names
extern const glConst GLActiveUniforms;

/// Draw primitives
extern const glConst GLLines;
extern const glConst GLLineStrip;
extern const glConst GLTriangles;
extern const glConst GLTriangleStrip;

/// Framebuffer attachment points
extern const glConst GLColorAttachment;
extern const glConst GLDepthAttachment;
extern const glConst GLStencilAttachment;
extern const glConst GLDepthStencilAttachment;

/// Framebuffer status
extern const glConst GLFramebufferComplete;
}  // namespace gl_const
