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
  float2 a_position [[attribute(0)]];
  float2 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} RulerVertex_T;

typedef struct
{
  float4 position [[position]];
  half4 color;
} RulerFragment_T;

vertex RulerFragment_T vsRuler(const RulerVertex_T in [[stage_in]],
                               texture2d<half> u_colorTex [[texture(0)]],
                               sampler u_colorTexSampler [[sampler(0)]],
                               constant Uniforms_T & uniforms [[buffer(1)]])
{
  RulerFragment_T out;
  float2 p = uniforms.u_position + in.a_position + uniforms.u_length * in.a_normal;
  out.position = float4(p, 0.0, 1.0) * uniforms.u_projection;
  half4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

fragment half4 fsRuler(const RulerFragment_T in [[stage_in]])
{
  return in.color;
}

// TextStaticOutlinedGui / TextOutlinedGui

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_colorTexCoord [[attribute(1)]];
  float2 a_outlineColorTexCoord [[attribute(2)]];
  float2 a_normal [[attribute(3)]];
  float2 a_maskTexCoord [[attribute(4)]];
} TextStaticOutlinedGuiVertex_T;

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_colorTexCoord [[attribute(1)]];
  float2 a_outlineColorTexCoord [[attribute(2)]];
  float2 a_normal [[attribute(3)]];
  float2 a_maskTexCoord [[attribute(4)]];
} TextOutlinedGuiVertex_T;

typedef struct
{
  float4 position [[position]];
  half4 glyphColor;
  float2 maskTexCoord;
} TextOutlinedGuiFragment_T;

TextOutlinedGuiFragment_T ComputeTextOutlinedGuiVertex(constant Uniforms_T & uniforms, float3 a_position, float2 a_normal,
                                                       float2 a_colorTexCoord, float2 a_outlineColorTexCoord,
                                                       float2 a_maskTexCoord, texture2d<half> u_colorTex,
                                                       sampler u_colorTexSampler)
{
  constexpr float kBaseDepthShift = -10.0;
  
  TextOutlinedGuiFragment_T out;
  
  float isOutline = step(0.5, uniforms.u_isOutlinePass);
  float depthShift = kBaseDepthShift * isOutline;
  
  float4 pos = (float4(a_position, 1.0) + float4(0.0, 0.0, depthShift, 0.0)) * uniforms.u_modelView;
  float4 shiftedPos = float4(a_normal, 0.0, 0.0) + pos;
  out.position = shiftedPos * uniforms.u_projection;
  out.glyphColor = u_colorTex.sample(u_colorTexSampler,
                                     mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline));
  out.maskTexCoord = a_maskTexCoord;
  return out;
}

vertex TextOutlinedGuiFragment_T vsTextStaticOutlinedGui(const TextStaticOutlinedGuiVertex_T in [[stage_in]],
                                                         constant Uniforms_T & uniforms [[buffer(1)]],
                                                         texture2d<half> u_colorTex [[texture(0)]],
                                                         sampler u_colorTexSampler [[sampler(0)]])
{
  return ComputeTextOutlinedGuiVertex(uniforms, in.a_position, in.a_normal, in.a_colorTexCoord,
                                      in.a_outlineColorTexCoord, in.a_maskTexCoord,
                                      u_colorTex, u_colorTexSampler);
}

vertex TextOutlinedGuiFragment_T vsTextOutlinedGui(const TextOutlinedGuiVertex_T in [[stage_in]],
                                                   constant Uniforms_T & uniforms [[buffer(2)]],
                                                   texture2d<half> u_colorTex [[texture(0)]],
                                                   sampler u_colorTexSampler [[sampler(0)]])
{
  return ComputeTextOutlinedGuiVertex(uniforms, in.a_position, in.a_normal, in.a_colorTexCoord,
                                      in.a_outlineColorTexCoord, in.a_maskTexCoord,
                                      u_colorTex, u_colorTexSampler);
}

fragment half4 fsTextOutlinedGui(const TextOutlinedGuiFragment_T in [[stage_in]],
                                 constant Uniforms_T & uniforms [[buffer(0)]],
                                 texture2d<float> u_maskTex [[texture(0)]],
                                 sampler u_maskTexSampler [[sampler(0)]])
{
  half4 glyphColor = in.glyphColor;
  float dist = u_maskTex.sample(u_maskTexSampler, in.maskTexCoord).a;
  float2 contrastGamma = uniforms.u_contrastGamma;
  float alpha = smoothstep(contrastGamma.x - contrastGamma.y, contrastGamma.x + contrastGamma.y, dist);
  glyphColor.a *= (alpha * uniforms.u_opacity);
  return glyphColor;
}

// TexturingGui

typedef struct
{
  float2 a_position [[attribute(0)]];
  float2 a_texCoords [[attribute(1)]];
} TexturingGuiVertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} TexturingGuiFragment_T;

vertex TexturingGuiFragment_T vsTexturingGui(const TexturingGuiVertex_T in [[stage_in]],
                                             constant Uniforms_T & uniforms [[buffer(1)]])
{
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
