#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float4x4 u_modelView;
  float4x4 u_projection;
  packed_float2 u_contrastGamma;
  packed_float2 u_position;
  float u_isOutlinePass;
  float u_opacity;
  float u_length;
} Uniforms_T;

// Ruler

typedef struct
{
  packed_float2 a_position;
  packed_float2 a_normal;
  packed_float2 a_texCoords;
} RulerVertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} RulerFragment_T;

vertex RulerFragment_T vsRuler(device const RulerVertex_T * vertices [[buffer(0)]],
                               constant Uniforms_T & uniforms [[buffer(1)]],
                               ushort vid [[vertex_id]])
{
  RulerVertex_T const in = vertices[vid];
  RulerFragment_T out;
  float2 p = uniforms.u_position + in.a_position + uniforms.u_length * in.a_normal;
  out.position = float4(p, 0.0, 1.0) * uniforms.u_projection;
  out.texCoords = in.a_texCoords;
  return out;
}

fragment float4 fsRuler(const RulerFragment_T in [[stage_in]],
                        constant Uniforms_T & uniforms [[buffer(0)]],
                        texture2d<float> u_colorTex [[texture(0)]],
                        sampler u_colorTexSampler [[sampler(0)]])
{
  float4 color = u_colorTex.sample(u_colorTexSampler, in.texCoords);
  color.a *= uniforms.u_opacity;
  return color;
}

// TextStaticOutlinedGui / TextOutlinedGui

typedef struct
{
  packed_float3 a_position;
  packed_float2 a_normal;
  packed_float2 a_colorTexCoord;
  packed_float2 a_outlineColorTexCoord;
  packed_float2 a_maskTexCoord;
} TextStaticOutlinedGuiVertex_T;

typedef struct
{
  packed_float3 a_position;
  packed_float2 a_colorTexCoord;
  packed_float2 a_outlineColorTexCoord;
} TextOutlinedGuiVertex0_T;

typedef struct
{
  packed_float2 a_normal;
  packed_float2 a_maskTexCoord;
} TextOutlinedGuiVertex1_T;

typedef struct
{
  float4 position [[position]];
  float2 colorTexCoords;
  float2 maskTexCoord;
} TextOutlinedGuiFragment_T;

TextOutlinedGuiFragment_T ComputeTextOutlinedGuiVertex(constant Uniforms_T & uniforms, float3 a_position, float2 a_normal,
                                                       float2 a_colorTexCoord, float2 a_outlineColorTexCoord,
                                                       float2 a_maskTexCoord)
{
  constexpr float kBaseDepthShift = -10.0;
  
  TextOutlinedGuiFragment_T out;
  
  float isOutline = step(0.5, uniforms.u_isOutlinePass);
  float depthShift = kBaseDepthShift * isOutline;
  
  float4 pos = (float4(a_position, 1.0) + float4(0.0, 0.0, depthShift, 0.0)) * uniforms.u_modelView;
  float4 shiftedPos = float4(a_normal, 0.0, 0.0) + pos;
  out.position = shiftedPos * uniforms.u_projection;
  out.colorTexCoords = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline);
  out.maskTexCoord = a_maskTexCoord;
  return out;
}

vertex TextOutlinedGuiFragment_T vsTextStaticOutlinedGui(device const TextStaticOutlinedGuiVertex_T * vertices [[buffer(0)]],
                                                         constant Uniforms_T & uniforms [[buffer(1)]],
                                                         ushort vid [[vertex_id]])
{
  TextStaticOutlinedGuiVertex_T const in = vertices[vid];
  return ComputeTextOutlinedGuiVertex(uniforms, in.a_position, in.a_normal, in.a_colorTexCoord, in.a_outlineColorTexCoord,
                                      in.a_maskTexCoord);
}

vertex TextOutlinedGuiFragment_T vsTextOutlinedGui(device const TextOutlinedGuiVertex0_T * vertices0 [[buffer(0)]],
                                                   device const TextOutlinedGuiVertex1_T * vertices1 [[buffer(1)]],
                                                   constant Uniforms_T & uniforms [[buffer(2)]],
                                                   ushort vid [[vertex_id]])
{
  TextOutlinedGuiVertex0_T const in0 = vertices0[vid];
  TextOutlinedGuiVertex1_T const in1 = vertices1[vid];
  return ComputeTextOutlinedGuiVertex(uniforms, in0.a_position, in1.a_normal, in0.a_colorTexCoord,
                                      in0.a_outlineColorTexCoord, in1.a_maskTexCoord);
}

fragment float4 fsTextOutlinedGui(const TextOutlinedGuiFragment_T in [[stage_in]],
                                  constant Uniforms_T & uniforms [[buffer(0)]],
                                  texture2d<float> u_colorTex [[texture(0)]],
                                  sampler u_colorTexSampler [[sampler(0)]],
                                  texture2d<float> u_maskTex [[texture(1)]],
                                  sampler u_maskTexSampler [[sampler(1)]])
{
  float4 glyphColor = u_colorTex.sample(u_colorTexSampler, in.colorTexCoords);
  float dist = u_maskTex.sample(u_maskTexSampler, in.maskTexCoord).a;
  float2 contrastGamma = uniforms.u_contrastGamma;
  float alpha = smoothstep(contrastGamma.x - contrastGamma.y, contrastGamma.x + contrastGamma.y, dist);
  glyphColor.a *= (alpha * uniforms.u_opacity);
  return glyphColor;
}

// TexturingGui

typedef struct
{
  packed_float2 a_position;
  packed_float2 a_texCoords;
} TexturingGuiVertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} TexturingGuiFragment_T;

vertex TexturingGuiFragment_T vsTexturingGui(device const TexturingGuiVertex_T * vertices [[buffer(0)]],
                                             constant Uniforms_T & uniforms [[buffer(1)]],
                                             ushort vid [[vertex_id]])
{
  TexturingGuiVertex_T const in = vertices[vid];
  TexturingGuiFragment_T out;
  out.position = float4(in.a_position, 0.0, 1.0) * uniforms.u_modelView * uniforms.u_projection;
  out.texCoords = in.a_texCoords;
  return out;
}

fragment float4 fsTexturingGui(const TexturingGuiFragment_T in [[stage_in]],
                               constant Uniforms_T & uniforms [[buffer(0)]],
                               texture2d<float> u_colorTex [[texture(0)]],
                               sampler u_colorTexSampler [[sampler(0)]])
{
  float4 color = u_colorTex.sample(u_colorTexSampler, in.texCoords);
  color.a *= uniforms.u_opacity;
  return color;
}
