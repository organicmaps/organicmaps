#include <metal_stdlib>
#include <simd/simd.h>
#include "shaders_lib.h"
using namespace metal;

typedef struct
{
  float4x4 u_modelView;
  float4x4 u_projection;
  float4x4 u_pivotTransform;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
  packed_float2 u_contrastGamma;
} Uniforms_T;

// Area/AreaOutline

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_texCoords [[attribute(1)]];
} AreaVertex_T;

typedef struct
{
  float4 position [[position]];
  half4 color;
} AreaFragment_T;

vertex AreaFragment_T vsArea(const AreaVertex_T in [[stage_in]],
                             constant Uniforms_T & uniforms [[buffer(1)]],
                             texture2d<half> u_colorTex [[texture(0)]],
                             sampler u_colorTexSampler [[sampler(0)]])
{
  AreaFragment_T out;
  float4 pos = float4(in.a_position, 1.0) * uniforms.u_modelView * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  half4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

fragment half4 fsArea(const AreaFragment_T in [[stage_in]])
{
  return in.color;
}

// Area3d

typedef struct
{
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} Area3dVertex_T;

typedef struct
{
  float4 position [[position]];
  half4 color;
  float intensity;
} Area3dFragment_T;

vertex Area3dFragment_T vsArea3d(const Area3dVertex_T in [[stage_in]],
                                 constant Uniforms_T & uniforms [[buffer(1)]],
                                 texture2d<half> u_colorTex [[texture(0)]],
                                 sampler u_colorTexSampler [[sampler(0)]])
{
  constexpr float4 kNormalizedLightDir = float4(0.3162, 0.0, 0.9486, 0.0);
  
  Area3dFragment_T out;

  float4 pos = float4(in.a_position, 1.0) * uniforms.u_modelView;
  
  float4 normal = float4(in.a_position + in.a_normal, 1.0) * uniforms.u_modelView;
  normal.xyw = (normal * uniforms.u_projection).xyw;
  normal.z = normal.z * uniforms.u_zScale;
  
  pos.xyw = (pos * uniforms.u_projection).xyw;
  pos.z = in.a_position.z * uniforms.u_zScale;
  
  float4 normDir = normal - pos;
  if (dot(normDir, normDir) != 0.0)
    out.intensity = max(0.0, -dot(kNormalizedLightDir, normalize(normDir)));
  else
    out.intensity = 0.0;
  
  out.position = uniforms.u_pivotTransform * pos;
  
  half4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a = (half)uniforms.u_opacity;
  out.color = color;
  
  return out;
}

fragment half4 fsArea3d(const Area3dFragment_T in [[stage_in]])
{
  return half4(in.color.rgb * (in.intensity * 0.2 + 0.8), in.color.a);
}

// Area3dOutline

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_texCoords [[attribute(1)]];
} Area3dOutlineVertex_T;

vertex AreaFragment_T vsArea3dOutline(const Area3dOutlineVertex_T in [[stage_in]],
                                      constant Uniforms_T & uniforms [[buffer(1)]],
                                      texture2d<half> u_colorTex [[texture(0)]],
                                      sampler u_colorTexSampler [[sampler(0)]])
{
  AreaFragment_T out;
  
  float4 pos = float4(in.a_position, 1.0) * uniforms.u_modelView;
  pos.xyw = (pos * uniforms.u_projection).xyw;
  pos.z = in.a_position.z * uniforms.u_zScale;
  out.position = uniforms.u_pivotTransform * pos;
  
  half4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

// HatchingArea

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_colorTexCoords [[attribute(1)]];
  float2 a_maskTexCoords [[attribute(2)]];
} HatchingAreaVertex_T;

typedef struct
{
  float4 position [[position]];
  half4 color;
  float2 maskTexCoords;
} HatchingAreaFragment_T;

vertex HatchingAreaFragment_T vsHatchingArea(const HatchingAreaVertex_T in [[stage_in]],
                                             constant Uniforms_T & uniforms [[buffer(1)]],
                                             texture2d<half> u_colorTex [[texture(0)]],
                                             sampler u_colorTexSampler [[sampler(0)]])
{
  HatchingAreaFragment_T out;
  
  float4 pos = float4(in.a_position, 1.0) * uniforms.u_modelView * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  out.maskTexCoords = in.a_maskTexCoords;
  half4 color = u_colorTex.sample(u_colorTexSampler, in.a_colorTexCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

fragment half4 fsHatchingArea(const HatchingAreaFragment_T in [[stage_in]],
                              texture2d<half> u_maskTex [[texture(0)]],
                              sampler u_maskTexSampler [[sampler(0)]])
{
  return in.color * u_maskTex.sample(u_maskTexSampler, in.maskTexCoords);
}

// CirclePoint

typedef struct
{
  float3 a_normal [[attribute(0)]];
  float3 a_position [[attribute(1)]];
  float4 a_color [[attribute(2)]];
} CirclePointVertex_T;

typedef struct
{
  float4 position [[position]];
  float3 radius;
  float4 color;
} CirclePointFragment_T;

vertex CirclePointFragment_T vsCirclePoint(const CirclePointVertex_T in [[stage_in]],
                                           constant Uniforms_T & uniforms [[buffer(2)]])
{
  CirclePointFragment_T out;
  
  float3 radius = in.a_normal * in.a_position.z;
  float4 pos = float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView;
  float4 shiftedPos = float4(radius.xy, 0.0, 0.0) + pos;
  out.position = ApplyPivotTransform(shiftedPos * uniforms.u_projection, uniforms.u_pivotTransform, 0.0);
  out.radius = radius;
  out.color = in.a_color;

  return out;
}

fragment float4 fsCirclePoint(const CirclePointFragment_T in [[stage_in]],
                              constant Uniforms_T & uniforms [[buffer(0)]])
{
  constexpr float kAntialiasingScalar = 0.9;
  
  float d = dot(in.radius.xy, in.radius.xy);
  float4 color = in.color;
  float aaRadius = in.radius.z * kAntialiasingScalar;
  float stepValue = 1.0 - smoothstep(aaRadius * aaRadius, in.radius.z * in.radius.z, d);
  color.a *= (uniforms.u_opacity * stepValue);
  return color;
}

// Line

typedef struct
{
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} LineVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
  float2 halfLength;
} LineFragment_T;

vertex LineFragment_T vsLine(const LineVertex_T in [[stage_in]],
                             constant Uniforms_T & uniforms [[buffer(1)]],
                             texture2d<float> u_colorTex [[texture(0)]],
                             sampler u_colorTexSampler [[sampler(0)]])
{
  LineFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float halfWidth = length(normal);
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  if (halfWidth != 0.0)
  {
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + normal,
                                                    uniforms.u_modelView, halfWidth);
  }
  
  out.halfLength = float2(sign(in.a_normal.z) * halfWidth, abs(in.a_normal.z));
  
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  
  float4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

fragment float4 fsLine(const LineFragment_T in [[stage_in]])
{
  constexpr float kAntialiasingPixelsCount = 2.5;
  
  float currentW = abs(in.halfLength.x);
  float diff = in.halfLength.y - currentW;
  
  float4 color = in.color;
  color.a *= mix(0.3, 1.0, saturate(diff / kAntialiasingPixelsCount));
  return color;
}

// DashedLine

typedef struct
{
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
  float2 a_colorTexCoord [[attribute(2)]];
  float4 a_maskTexCoord [[attribute(3)]];
} DashedLineVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
  float2 maskTexCoord;
  float2 halfLength;
} DashedLineFragment_T;

vertex DashedLineFragment_T vsDashedLine(const DashedLineVertex_T in [[stage_in]],
                                         constant Uniforms_T & uniforms [[buffer(1)]],
                                         texture2d<float> u_colorTex [[texture(0)]],
                                         sampler u_colorTexSampler [[sampler(0)]])
{
  DashedLineFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float halfWidth = length(normal);
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  if (halfWidth != 0.0)
  {
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + normal,
                                                    uniforms.u_modelView, halfWidth);
  }
  
  float uOffset = min(length(float4(kShapeCoordScalar, 0.0, 0.0, 0.0) * uniforms.u_modelView) * in.a_maskTexCoord.x, 1.0);
  out.maskTexCoord = float2(in.a_maskTexCoord.y + uOffset * in.a_maskTexCoord.z, in.a_maskTexCoord.w);
  
  out.halfLength = float2(sign(in.a_normal.z) * halfWidth, abs(in.a_normal.z));
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);

  float4 color = u_colorTex.sample(u_colorTexSampler, in.a_colorTexCoord);
  color.a *= uniforms.u_opacity;
  out.color = color;
  return out;
}

fragment float4 fsDashedLine(const DashedLineFragment_T in [[stage_in]],
                             texture2d<float> u_maskTex [[texture(0)]],
                             sampler u_maskTexSampler [[sampler(0)]])
{
  constexpr float kAntialiasingPixelsCount = 2.5;
  
  float4 color = in.color;
  color.a *= u_maskTex.sample(u_maskTexSampler, in.maskTexCoord).a;
  
  float currentW = abs(in.halfLength.x);
  float diff = in.halfLength.y - currentW;
  color.a *= mix(0.3, 1.0, saturate(diff / kAntialiasingPixelsCount));
  return color;
}

// CapJoin

typedef struct
{
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} CapJoinVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
  float3 radius;
} CapJoinFragment_T;

typedef struct
{
  float4 color [[color(0)]];
  float depth [[depth(any)]];
} CapJoinFragment_Output;

vertex CapJoinFragment_T vsCapJoin(const CapJoinVertex_T in [[stage_in]],
                                   constant Uniforms_T & uniforms [[buffer(1)]],
                                   texture2d<float> u_colorTex [[texture(0)]],
                                   sampler u_colorTexSampler [[sampler(0)]])
{
  CapJoinFragment_T out;
  
  float4 p = float4(in.a_position, 1.0) * uniforms.u_modelView;
  float4 pos = float4(in.a_normal.xy, 0.0, 0.0) + p;
  out.position = ApplyPivotTransform(pos * uniforms.u_projection, uniforms.u_pivotTransform, 0.0);
  out.radius = in.a_normal;
  float4 color = u_colorTex.sample(u_colorTexSampler, in.a_texCoords);
  color.a *= uniforms.u_opacity;
  out.color = color;
  
  return out;
}

fragment CapJoinFragment_Output fsCapJoin(const CapJoinFragment_T in [[stage_in]])
{
  constexpr float kAntialiasingPixelsCount = 2.5;
  
  CapJoinFragment_Output out;
  
  float smallRadius = in.radius.z - kAntialiasingPixelsCount;
  float stepValue = 1.0 - smoothstep(smallRadius * smallRadius, in.radius.z * in.radius.z,
                                     in.radius.x * in.radius.x + in.radius.y * in.radius.y);
  out.color = in.color;
  out.color.a *= stepValue;
  
  if (out.color.a < 0.001)
    out.depth = 1.0;
  else
    out.depth = in.position.z;
  
  return out;
}

// PathSymbol

typedef struct
{
  float4 a_position [[attribute(0)]];
  float2 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} PathSymbolVertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} PathSymbolFragment_T;

vertex PathSymbolFragment_T vsPathSymbol(const PathSymbolVertex_T in [[stage_in]],
                                         constant Uniforms_T & uniforms [[buffer(1)]])
{
  PathSymbolFragment_T out;

  float4 pos = float4(in.a_position.xyz, 1.0) * uniforms.u_modelView;
  
  float normalLen = length(in.a_normal);
  float4 n = float4(in.a_position.xy + in.a_normal * kShapeCoordScalar, 0.0, 0.0) * uniforms.u_modelView;
  float4 norm;
  if (dot(n, n) != 0.0)
    norm = normalize(n) * normalLen;
  else
    norm = float4(0.0, 0.0, 0.0, 0.0);
  
  float4 shiftedPos = norm + pos;
  out.position = ApplyPivotTransform(shiftedPos * uniforms.u_projection, uniforms.u_pivotTransform, 0.0);
  out.texCoords = in.a_texCoords;
  
  return out;
}

fragment float4 fsPathSymbol(const PathSymbolFragment_T in [[stage_in]],
                             constant Uniforms_T & uniforms [[buffer(0)]],
                             texture2d<float> u_colorTex [[texture(0)]],
                             sampler u_colorTexSampler [[sampler(0)]])
{
  float4 color = u_colorTex.sample(u_colorTexSampler, in.texCoords);
  color.a *= uniforms.u_opacity;
  return color;
}
