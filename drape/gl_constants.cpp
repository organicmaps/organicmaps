#include "drape/gl_constants.hpp"
#include "drape/gl_includes.hpp"

#if !defined(GL_RGBA8_OES)
#define GL_RGBA8_OES 0x8058
#endif

#if !defined(GL_RGBA4_OES)
#define GL_RGBA4_OES 0x8056
#endif

#if !defined(GL_ALPHA8_OES)
#define GL_ALPHA8_OES 0x803C
#endif

#if !defined(GL_LUMINANCE8_OES)
#define GL_LUMINANCE8_OES 0x8040
#endif

#if !defined(GL_LUMINANCE8_ALPHA8_OES)
#define GL_LUMINANCE8_ALPHA8_OES 0x8045
#endif

#if !defined(GL_LUMINANCE8_ALPHA4_OES)
#define GL_LUMINANCE8_ALPHA4_OES 0x8043
#endif

#if !defined(GL_LUMINANCE)
#define GL_LUMINANCE 0x1909
#endif

#if !defined(GL_LUMINANCE_ALPHA)
#define GL_LUMINANCE_ALPHA 0x190A
#endif

#if defined(GL_WRITE_ONLY)
#define WRITE_ONLY_DEF GL_WRITE_ONLY
#elif defined(GL_WRITE_ONLY_OES)
#define WRITE_ONLY_DEF GL_WRITE_ONLY_OES
#else
#define WRITE_ONLY_DEF 0x88B9
#endif

#if defined(GL_READ_ONLY)
#define READ_ONLY_DEF GL_READ_ONLY
#else
#define READ_ONLY_DEF 0x88B8
#endif

#if defined(GL_MAP_READ_BIT_EXT)
#define READ_BIT_DEF GL_MAP_READ_BIT_EXT
#else
#define READ_BIT_DEF 0x0001
#endif

#if defined(GL_MAP_WRITE_BIT_EXT)
#define WRITE_BIT_DEF GL_MAP_WRITE_BIT_EXT
#else
#define WRITE_BIT_DEF 0x0002
#endif

#if defined(GL_MAP_INVALIDATE_RANGE_BIT_EXT)
#define INVALIDATE_RANGE_BIT_DEF GL_MAP_INVALIDATE_RANGE_BIT_EXT
#else
#define INVALIDATE_RANGE_BIT_DEF 0x0004
#endif

#if defined(GL_MAP_INVALIDATE_BUFFER_BIT_EXT)
#define INVALIDATE_BUFFER_BIT_DEF GL_MAP_INVALIDATE_BUFFER_BIT_EXT
#else
#define INVALIDATE_BUFFER_BIT_DEF 0x0008
#endif

#if defined(GL_MAP_FLUSH_EXPLICIT_BIT_EXT)
#define FLUSH_EXPLICIT_BIT_DEF GL_MAP_FLUSH_EXPLICIT_BIT_EXT
#else
#define FLUSH_EXPLICIT_BIT_DEF 0x0010
#endif

#if defined(GL_MAP_UNSYNCHRONIZED_BIT_EXT)
#define UNSYNCHRONIZED_BIT_DEF GL_MAP_UNSYNCHRONIZED_BIT_EXT
#else
#define UNSYNCHRONIZED_BIT_DEF 0x0020
#endif

#if !defined(GL_FUNC_ADD)
#define GL_FUNC_ADD 0x8006
#endif

#if !defined(GL_FUNC_SUBTRACT)
#define GL_FUNC_SUBTRACT 0x800A
#endif

#if !defined(GL_FUNC_REVERSE_SUBTRACT)
#define GL_FUNC_REVERSE_SUBTRACT 0x800B
#endif

namespace gl_const
{

constexpr glConst GLUnpackAlignment = GL_UNPACK_ALIGNMENT;

constexpr glConst GLRenderer = GL_RENDERER;
constexpr glConst GLVendor = GL_VENDOR;
constexpr glConst GLVersion = GL_VERSION;

#ifdef GL_VERSION_3_0
constexpr glConst glContextFlags = GL_CONTEXT_FLAGS;
#else
constexpr glConst glContextFlags = 0;
#endif

constexpr glConst GLColorBit = GL_COLOR_BUFFER_BIT;
constexpr glConst GLDepthBit = GL_DEPTH_BUFFER_BIT;
constexpr glConst GLStencilBit = GL_STENCIL_BUFFER_BIT;

constexpr glConst GLMaxFragmentTextures = GL_MAX_TEXTURE_IMAGE_UNITS;
constexpr glConst GLMaxVertexTextures = GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS;
constexpr glConst GLMaxTextureSize = GL_MAX_TEXTURE_SIZE;

constexpr glConst GLArrayBuffer = GL_ARRAY_BUFFER;
constexpr glConst GLElementArrayBuffer = GL_ELEMENT_ARRAY_BUFFER;
constexpr glConst GLPixelBufferWrite = GL_PIXEL_UNPACK_BUFFER;

constexpr glConst GLBufferSize = GL_BUFFER_SIZE;
constexpr glConst GLBufferUsage = GL_BUFFER_USAGE;

constexpr glConst GLWriteOnly = WRITE_ONLY_DEF;
constexpr glConst GLReadOnly = READ_ONLY_DEF;

constexpr glConst GLReadBufferBit = READ_BIT_DEF;
constexpr glConst GLWriteBufferBit = WRITE_BIT_DEF;
constexpr glConst GLInvalidateRange = INVALIDATE_RANGE_BIT_DEF;
constexpr glConst GLInvalidateBuffer = INVALIDATE_BUFFER_BIT_DEF;
constexpr glConst GLFlushExplicit = FLUSH_EXPLICIT_BIT_DEF;
constexpr glConst GLUnsynchronized = UNSYNCHRONIZED_BIT_DEF;

constexpr glConst GLStaticDraw = GL_STATIC_DRAW;
constexpr glConst GLStreamDraw = GL_STREAM_DRAW;
constexpr glConst GLDynamicDraw = GL_DYNAMIC_DRAW;

constexpr glConst GLVertexShader = GL_VERTEX_SHADER;
constexpr glConst GLFragmentShader = GL_FRAGMENT_SHADER;
constexpr glConst GLCurrentProgram = GL_CURRENT_PROGRAM;

constexpr glConst GLRGBA = GL_RGBA;
constexpr glConst GLRGB = GL_RGB;
constexpr glConst GLAlpha = GL_ALPHA;
constexpr glConst GLLuminance = GL_LUMINANCE;
constexpr glConst GLAlphaLuminance = GL_LUMINANCE_ALPHA;
constexpr glConst GLDepthComponent = GL_DEPTH_COMPONENT;
constexpr glConst GLDepthStencil = GL_DEPTH_STENCIL;

constexpr glConst GLRGBA8 = GL_RGBA8_OES;
constexpr glConst GLRGBA4 = GL_RGBA4_OES;
constexpr glConst GLAlpha8 = GL_ALPHA8_OES;
constexpr glConst GLLuminance8 = GL_LUMINANCE8_OES;
constexpr glConst GLAlphaLuminance8 = GL_LUMINANCE8_ALPHA8_OES;
constexpr glConst GLAlphaLuminance4 = GL_LUMINANCE8_ALPHA4_OES;
constexpr glConst GLRed = GL_RED;
constexpr glConst GLRedGreen = GL_RG;

constexpr glConst GL8BitOnChannel = GL_UNSIGNED_BYTE;
constexpr glConst GL4BitOnChannel = GL_UNSIGNED_SHORT_4_4_4_4;

constexpr glConst GLTexture2D = GL_TEXTURE_2D;

constexpr glConst GLTexture0 = GL_TEXTURE0;

constexpr glConst GLMinFilter = GL_TEXTURE_MIN_FILTER;
constexpr glConst GLMagFilter = GL_TEXTURE_MAG_FILTER;
constexpr glConst GLWrapS = GL_TEXTURE_WRAP_S;
constexpr glConst GLWrapT = GL_TEXTURE_WRAP_T;

constexpr glConst GLRepeat = GL_REPEAT;
constexpr glConst GLMirroredRepeat = GL_MIRRORED_REPEAT;
constexpr glConst GLClampToEdge = GL_CLAMP_TO_EDGE;

constexpr glConst GLLinear = GL_LINEAR;
constexpr glConst GLNearest = GL_NEAREST;

constexpr glConst GLByteType = GL_BYTE;
constexpr glConst GLUnsignedByteType = GL_UNSIGNED_BYTE;
constexpr glConst GLShortType = GL_SHORT;
constexpr glConst GLUnsignedShortType = GL_UNSIGNED_SHORT;
constexpr glConst GLIntType = GL_INT;
constexpr glConst GLUnsignedIntType = GL_UNSIGNED_INT;
constexpr glConst GLFloatType = GL_FLOAT;
constexpr glConst GLUnsignedInt24_8Type = GL_UNSIGNED_INT_24_8;

constexpr glConst GLFloatVec2 = GL_FLOAT_VEC2;
constexpr glConst GLFloatVec3 = GL_FLOAT_VEC3;
constexpr glConst GLFloatVec4 = GL_FLOAT_VEC4;

constexpr glConst GLIntVec2 = GL_INT_VEC2;
constexpr glConst GLIntVec3 = GL_INT_VEC3;
constexpr glConst GLIntVec4 = GL_INT_VEC4;

constexpr glConst GLFloatMat4 = GL_FLOAT_MAT4;

constexpr glConst GLSampler2D = GL_SAMPLER_2D;

constexpr glConst GLAddBlend = GL_FUNC_ADD;
constexpr glConst GLSubstractBlend = GL_FUNC_SUBTRACT;
constexpr glConst GLReverseSubstrBlend = GL_FUNC_REVERSE_SUBTRACT;

constexpr glConst GLZero = GL_ZERO;
constexpr glConst GLOne = GL_ONE;
constexpr glConst GLSrcColor = GL_SRC_COLOR;
constexpr glConst GLOneMinusSrcColor = GL_ONE_MINUS_SRC_COLOR;
constexpr glConst GLDstColor = GL_DST_COLOR;
constexpr glConst GLOneMinusDstColor = GL_ONE_MINUS_DST_COLOR;
constexpr glConst GLSrcAlpha = GL_SRC_ALPHA;
constexpr glConst GLOneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA;
constexpr glConst GLDstAlpha = GL_DST_ALPHA;
constexpr glConst GLOneMinusDstAlpha = GL_ONE_MINUS_DST_ALPHA;

constexpr glConst GLDepthTest = GL_DEPTH_TEST;
constexpr glConst GLBlending = GL_BLEND;
constexpr glConst GLCullFace = GL_CULL_FACE;
constexpr glConst GLScissorTest = GL_SCISSOR_TEST;
constexpr glConst GLStencilTest = GL_STENCIL_TEST;

constexpr glConst GLDontCare = GL_DONT_CARE;
constexpr glConst GLTrue = GL_TRUE;
constexpr glConst GLFalse = GL_FALSE;

#ifdef GL_VERSION_4_3
constexpr glConst GLDebugOutput = GL_DEBUG_OUTPUT;
constexpr glConst GLDebugOutputSynchronous = GL_DEBUG_OUTPUT_SYNCHRONOUS;

constexpr glConst GLDebugSourceApi = GL_DEBUG_SOURCE_API;
constexpr glConst GLDebugSourceShaderCompiler = GL_DEBUG_SOURCE_SHADER_COMPILER;
constexpr glConst GLDebugSourceThirdParty = GL_DEBUG_SOURCE_THIRD_PARTY;
constexpr glConst GLDebugSourceApplication = GL_DEBUG_SOURCE_APPLICATION;
constexpr glConst GLDebugSourceOther = GL_DEBUG_SOURCE_OTHER;

constexpr glConst GLDebugTypeError = GL_DEBUG_TYPE_ERROR;
constexpr glConst GLDebugDeprecatedBehavior = GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR;
constexpr glConst GLDebugUndefinedBehavior = GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR;
constexpr glConst GLDebugPortability = GL_DEBUG_TYPE_PORTABILITY;
constexpr glConst GLDebugPerformance = GL_DEBUG_TYPE_PERFORMANCE;
constexpr glConst GLDebugOther = GL_DEBUG_TYPE_OTHER;

constexpr glConst GLDebugSeverityLow = GL_DEBUG_SEVERITY_LOW;
constexpr glConst GLDebugSeverityMedium = GL_DEBUG_SEVERITY_MEDIUM;
constexpr glConst GLDebugSeverityHigh = GL_DEBUG_SEVERITY_HIGH;
constexpr glConst GLDebugSeverityNotification = GL_DEBUG_SEVERITY_NOTIFICATION;

constexpr glConst glContextFlagDebugBit = GL_CONTEXT_FLAG_DEBUG_BIT;
#else
constexpr glConst GLDebugOutput = 0;
constexpr glConst GLDebugOutputSynchronous = 0;

constexpr glConst GLDebugSourceApi = 0;
constexpr glConst GLDebugSourceShaderCompiler = 0;
constexpr glConst GLDebugSourceThirdParty = 0;
constexpr glConst GLDebugSourceApplication = 0;
constexpr glConst GLDebugSourceOther = 0;

constexpr glConst GLDebugTypeError = 0;
constexpr glConst GLDebugDeprecatedBehavior = 0;
constexpr glConst GLDebugUndefinedBehavior = 0;
constexpr glConst GLDebugPortability = 0;
constexpr glConst GLDebugPerformance = 0;
constexpr glConst GLDebugOther = 0;

constexpr glConst GLDebugSeverityLow = 0;
constexpr glConst GLDebugSeverityMedium = 0;
constexpr glConst GLDebugSeverityHigh = 0;
constexpr glConst GLDebugSeverityNotification = 0;

constexpr glConst glContextFlagDebugBit = 0;
#endif

constexpr glConst GLClockwise = GL_CW;
constexpr glConst GLCounterClockwise = GL_CCW;

constexpr glConst GLFront = GL_FRONT;
constexpr glConst GLBack = GL_BACK;
constexpr glConst GLFrontAndBack = GL_FRONT_AND_BACK;

constexpr glConst GLNever = GL_NEVER;
constexpr glConst GLLess = GL_LESS;
constexpr glConst GLEqual = GL_EQUAL;
constexpr glConst GLLessOrEqual = GL_LEQUAL;
constexpr glConst GLGreat = GL_GREATER;
constexpr glConst GLNotEqual = GL_NOTEQUAL;
constexpr glConst GLGreatOrEqual = GL_GEQUAL;
constexpr glConst GLAlways = GL_ALWAYS;

constexpr glConst GLKeep = GL_KEEP;
constexpr glConst GLIncr = GL_INCR;
constexpr glConst GLDecr = GL_DECR;
constexpr glConst GLInvert = GL_INVERT;
constexpr glConst GLReplace = GL_REPLACE;
constexpr glConst GLIncrWrap = GL_INCR_WRAP;
constexpr glConst GLDecrWrap = GL_DECR_WRAP;

constexpr glConst GLActiveUniforms = GL_ACTIVE_UNIFORMS;

constexpr glConst GLLines = GL_LINES;
constexpr glConst GLLineStrip = GL_LINE_STRIP;
constexpr glConst GLTriangles = GL_TRIANGLES;
constexpr glConst GLTriangleStrip = GL_TRIANGLE_STRIP;

constexpr glConst GLColorAttachment = GL_COLOR_ATTACHMENT0;
constexpr glConst GLDepthAttachment = GL_DEPTH_ATTACHMENT;
constexpr glConst GLStencilAttachment = GL_STENCIL_ATTACHMENT;
constexpr glConst GLDepthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT;

constexpr glConst GLFramebufferComplete = GL_FRAMEBUFFER_COMPLETE;

}  // namespace gl_const
