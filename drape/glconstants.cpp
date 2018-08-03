#include "drape/glconstants.hpp"
#include "drape/glIncludes.hpp"

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

const glConst GLUnpackAlignment     = GL_UNPACK_ALIGNMENT;

const glConst GLRenderer            = GL_RENDERER;
const glConst GLVendor              = GL_VENDOR;
const glConst GLVersion             = GL_VERSION;

const glConst GLColorBit            = GL_COLOR_BUFFER_BIT;
const glConst GLDepthBit            = GL_DEPTH_BUFFER_BIT;
const glConst GLStencilBit          = GL_STENCIL_BUFFER_BIT;

const glConst GLMaxFragmentTextures = GL_MAX_TEXTURE_IMAGE_UNITS;
const glConst GLMaxVertexTextures   = GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS;
const glConst GLMaxTextureSize      = GL_MAX_TEXTURE_SIZE;

const glConst GLArrayBuffer         = GL_ARRAY_BUFFER;
const glConst GLElementArrayBuffer  = GL_ELEMENT_ARRAY_BUFFER;
const glConst GLPixelBufferWrite    = GL_PIXEL_UNPACK_BUFFER;

const glConst GLBufferSize          = GL_BUFFER_SIZE;
const glConst GLBufferUsage         = GL_BUFFER_USAGE;

const glConst GLWriteOnly           = WRITE_ONLY_DEF;
const glConst GLReadOnly            = READ_ONLY_DEF;

const glConst GLReadBufferBit       = READ_BIT_DEF;
const glConst GLWriteBufferBit      = WRITE_BIT_DEF;
const glConst GLInvalidateRange     = INVALIDATE_RANGE_BIT_DEF;
const glConst GLInvalidateBuffer    = INVALIDATE_BUFFER_BIT_DEF;
const glConst GLFlushExplicit       = FLUSH_EXPLICIT_BIT_DEF;
const glConst GLUnsynchronized      = UNSYNCHRONIZED_BIT_DEF;

const glConst GLStaticDraw          = GL_STATIC_DRAW;
const glConst GLStreamDraw          = GL_STREAM_DRAW;
const glConst GLDynamicDraw         = GL_DYNAMIC_DRAW;

const glConst GLVertexShader        = GL_VERTEX_SHADER;
const glConst GLFragmentShader      = GL_FRAGMENT_SHADER;
const glConst GLCurrentProgram      = GL_CURRENT_PROGRAM;

const glConst GLRGBA                = GL_RGBA;
const glConst GLRGB                 = GL_RGB;
const glConst GLAlpha               = GL_ALPHA;
const glConst GLLuminance           = GL_LUMINANCE;
const glConst GLAlphaLuminance      = GL_LUMINANCE_ALPHA;
const glConst GLDepthComponent      = GL_DEPTH_COMPONENT;
const glConst GLDepthStencil        = GL_DEPTH_STENCIL;

const glConst GLRGBA8               = GL_RGBA8_OES;
const glConst GLRGBA4               = GL_RGBA4_OES;
const glConst GLAlpha8              = GL_ALPHA8_OES;
const glConst GLLuminance8          = GL_LUMINANCE8_OES;
const glConst GLAlphaLuminance8     = GL_LUMINANCE8_ALPHA8_OES;
const glConst GLAlphaLuminance4     = GL_LUMINANCE8_ALPHA4_OES;
const glConst GLRed                 = GL_RED;
const glConst GLRedGreen            = GL_RG;

const glConst GL8BitOnChannel       = GL_UNSIGNED_BYTE;
const glConst GL4BitOnChannel       = GL_UNSIGNED_SHORT_4_4_4_4;

const glConst GLTexture2D           = GL_TEXTURE_2D;

const glConst GLTexture0            = GL_TEXTURE0;

const glConst GLMinFilter           = GL_TEXTURE_MIN_FILTER;
const glConst GLMagFilter           = GL_TEXTURE_MAG_FILTER;
const glConst GLWrapS               = GL_TEXTURE_WRAP_S;
const glConst GLWrapT               = GL_TEXTURE_WRAP_T;

const glConst GLRepeat              = GL_REPEAT;
const glConst GLMirroredRepeat      = GL_MIRRORED_REPEAT;
const glConst GLClampToEdge         = GL_CLAMP_TO_EDGE;

const glConst GLLinear              = GL_LINEAR;
const glConst GLNearest             = GL_NEAREST;

const glConst GLByteType            = GL_BYTE;
const glConst GLUnsignedByteType    = GL_UNSIGNED_BYTE;
const glConst GLShortType           = GL_SHORT;
const glConst GLUnsignedShortType   = GL_UNSIGNED_SHORT;
const glConst GLIntType             = GL_INT;
const glConst GLUnsignedIntType     = GL_UNSIGNED_INT;
const glConst GLFloatType           = GL_FLOAT;
const glConst GLUnsignedInt24_8Type = GL_UNSIGNED_INT_24_8;

const glConst GLFloatVec2           = GL_FLOAT_VEC2;
const glConst GLFloatVec3           = GL_FLOAT_VEC3;
const glConst GLFloatVec4           = GL_FLOAT_VEC4;

const glConst GLIntVec2             = GL_INT_VEC2;
const glConst GLIntVec3             = GL_INT_VEC3;
const glConst GLIntVec4             = GL_INT_VEC4;

const glConst GLFloatMat4           = GL_FLOAT_MAT4;

const glConst GLSampler2D           = GL_SAMPLER_2D;

const glConst GLAddBlend            = GL_FUNC_ADD;
const glConst GLSubstractBlend      = GL_FUNC_SUBTRACT;
const glConst GLReverseSubstrBlend  = GL_FUNC_REVERSE_SUBTRACT;

const glConst GLZero                = GL_ZERO;
const glConst GLOne                 = GL_ONE;
const glConst GLSrcColor            = GL_SRC_COLOR;
const glConst GLOneMinusSrcColor    = GL_ONE_MINUS_SRC_COLOR;
const glConst GLDstColor            = GL_DST_COLOR;
const glConst GLOneMinusDstColor    = GL_ONE_MINUS_DST_COLOR;
const glConst GLSrcAlpha            = GL_SRC_ALPHA;
const glConst GLOneMinusSrcAlpha    = GL_ONE_MINUS_SRC_ALPHA;
const glConst GLDstAlpha            = GL_DST_ALPHA;
const glConst GLOneMinusDstAlpha    = GL_ONE_MINUS_DST_ALPHA;

const glConst GLDepthTest           = GL_DEPTH_TEST;
const glConst GLBlending            = GL_BLEND;
const glConst GLCullFace            = GL_CULL_FACE;
const glConst GLScissorTest         = GL_SCISSOR_TEST;
const glConst GLStencilTest         = GL_STENCIL_TEST;

const glConst GLClockwise           = GL_CW;
const glConst GLCounterClockwise    = GL_CCW;

const glConst GLFront               = GL_FRONT;
const glConst GLBack                = GL_BACK;
const glConst GLFrontAndBack        = GL_FRONT_AND_BACK;

const glConst GLNever               = GL_NEVER;
const glConst GLLess                = GL_LESS;
const glConst GLEqual               = GL_EQUAL;
const glConst GLLessOrEqual         = GL_LEQUAL;
const glConst GLGreat               = GL_GREATER;
const glConst GLNotEqual            = GL_NOTEQUAL;
const glConst GLGreatOrEqual        = GL_GEQUAL;
const glConst GLAlways              = GL_ALWAYS;

const glConst GLKeep                = GL_KEEP;
const glConst GLIncr                = GL_INCR;
const glConst GLDecr                = GL_DECR;
const glConst GLInvert              = GL_INVERT;
const glConst GLReplace             = GL_REPLACE;
const glConst GLIncrWrap            = GL_INCR_WRAP;
const glConst GLDecrWrap            = GL_DECR_WRAP;

const glConst GLActiveUniforms      = GL_ACTIVE_UNIFORMS;

const glConst GLLines               = GL_LINES;
const glConst GLLineStrip           = GL_LINE_STRIP;
const glConst GLTriangles           = GL_TRIANGLES;
const glConst GLTriangleStrip       = GL_TRIANGLE_STRIP;

const glConst GLColorAttachment     = GL_COLOR_ATTACHMENT0;
const glConst GLDepthAttachment     = GL_DEPTH_ATTACHMENT;
const glConst GLStencilAttachment   = GL_STENCIL_ATTACHMENT;
const glConst GLDepthStencilAttachment = GL_DEPTH_STENCIL_ATTACHMENT;

const glConst GLFramebufferComplete = GL_FRAMEBUFFER_COMPLETE;

} // namespace GLConst
