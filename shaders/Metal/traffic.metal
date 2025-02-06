#include <metal_stdlib>
#include <simd/simd.h>
#include "shaders_lib.h"
using namespace metal;

typedef struct
{
  float4x4 u_modelView;
  float4x4 u_projection;
  float4x4 u_pivotTransform;
  float4 u_trafficParams;
  packed_float3 u_outlineColor;
  float u_dummy1; // alignment
  packed_float3 u_lightArrowColor;
  float u_dummy2; // alignment
  packed_float3 u_darkArrowColor;
  float u_dummy3; // alignment
  float u_outline;
  float u_opacity;
} Uniforms_T;

// Traffic

typedef struct
{
  float3 a_position [[attribute(0)]];
  float4 a_normal [[attribute(1)]];
  float4 a_colorTexCoord [[attribute(2)]];
} TrafficVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
  float2 maskTexCoord;
  float halfLength;
} TrafficFragment_T;

vertex TrafficFragment_T vsTraffic(const TrafficVertex_T in [[stage_in]],
                                   constant Uniforms_T & uniforms [[buffer(1)]],
                                   texture2d<float> u_colorTex [[texture(0)]],
                                   sampler u_colorTexSampler [[sampler(0)]])
{
  constexpr float kArrowVSize = 0.25;
  
  TrafficFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    float2 norm = normal * uniforms.u_trafficParams.x;
    if (in.a_normal.z < 0.0)
      norm = normal * uniforms.u_trafficParams.y;
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + norm,
                                                    uniforms.u_modelView, length(norm));
  }
  
  float uOffset = length(float4(kShapeCoordScalar, 0.0, 0.0, 0.0) * uniforms.u_modelView) * in.a_normal.w;
  out.color = u_colorTex.sample(u_colorTexSampler, in.a_colorTexCoord.xy);
  float v = mix(in.a_colorTexCoord.z, in.a_colorTexCoord.z + kArrowVSize, 0.5 * in.a_normal.z + 0.5);
  out.maskTexCoord = float2(uOffset * uniforms.u_trafficParams.z, v) * uniforms.u_trafficParams.w;
  out.maskTexCoord.x *= step(in.a_colorTexCoord.w, out.maskTexCoord.x);
  out.halfLength = in.a_normal.z;
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  
  return out;
}

fragment float4 fsTraffic(const TrafficFragment_T in [[stage_in]],
                          constant Uniforms_T & uniforms [[buffer(0)]],
                          texture2d<float> u_maskTex [[texture(0)]],
                          sampler u_maskTexSampler [[sampler(0)]])
{
  constexpr float kAntialiasingThreshold = 0.92;
  
  constexpr float kOutlineThreshold1 = 0.8;
  constexpr float kOutlineThreshold2 = 0.5;
  
  constexpr float kMaskOpacity = 0.7;
  
  float4 color = in.color;
  float alphaCode = color.a;
  float4 mask = u_maskTex.sample(u_maskTexSampler, in.maskTexCoord);
  color.a = uniforms.u_opacity * (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(in.halfLength)));
  color.rgb = mix(color.rgb, mask.rgb * mix(uniforms.u_lightArrowColor, uniforms.u_darkArrowColor, step(alphaCode, 0.6)), mask.a * kMaskOpacity);
  if (uniforms.u_outline > 0.0)
  {
    float3 outlineColor = uniforms.u_outlineColor;
    color.rgb = mix(color.rgb, outlineColor, step(kOutlineThreshold1, abs(in.halfLength)));
    color.rgb = mix(color.rgb, outlineColor, smoothstep(kOutlineThreshold2, kOutlineThreshold1, abs(in.halfLength)));
  }
  
  return color;
}

// TrafficLine

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_colorTexCoord [[attribute(1)]];
} TrafficLineVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
} TrafficLineFragment_T;

vertex TrafficLineFragment_T vsTrafficLine(const TrafficLineVertex_T in [[stage_in]],
                                           constant Uniforms_T & uniforms [[buffer(1)]],
                                           texture2d<float> u_colorTex [[texture(0)]],
                                           sampler u_colorTexSampler [[sampler(0)]])
{
  TrafficLineFragment_T out;
  
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.color = float4(u_colorTex.sample(u_colorTexSampler, in.a_colorTexCoord).rgb, uniforms.u_opacity);
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  
  return out;
}

fragment float4 fsTrafficLine(const TrafficLineFragment_T in [[stage_in]])
{
  return in.color;
}

// TrafficCircle

typedef struct
{
  float4 a_position [[attribute(0)]];
  float4 a_normal [[attribute(1)]];
  float2 a_colorTexCoord [[attribute(2)]];
} TrafficCircleVertex_T;

typedef struct
{
  float4 position [[position]];
  float4 color;
  float3 radius;
} TrafficCircleFragment_T;

vertex TrafficCircleFragment_T vsTrafficCircle(const TrafficCircleVertex_T in [[stage_in]],
                                               constant Uniforms_T & uniforms [[buffer(1)]],
                                               texture2d<float> u_colorTex [[texture(0)]],
                                               sampler u_colorTexSampler [[sampler(0)]])
{
  TrafficCircleFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  int index = int(in.a_position.w);
  
  float3 leftSizes = uniforms.u_lightArrowColor;
  float leftSize = leftSizes[index];
  
  float3 rightSizes = uniforms.u_darkArrowColor;
  float rightSize = rightSizes[index];
  
  if (dot(normal, normal) != 0.0)
  {
    // offset by normal = rightVec * (rightSize - leftSize) / 2
    float2 norm = normal * 0.5 * (rightSize - leftSize);
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + norm,
                                                    uniforms.u_modelView, length(norm));
  }
  // radius = (leftSize + rightSize) / 2
  out.radius = float3(in.a_normal.zw, 1.0) * 0.5 * (leftSize + rightSize);
  
  float2 finalPos = transformedAxisPos + out.radius.xy;
  out.color = u_colorTex.sample(u_colorTexSampler, in.a_colorTexCoord);
  float4 pos = float4(finalPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  
  return out;
}

typedef struct
{
  float4 color [[color(0)]];
  float depth [[depth(any)]];
} TrafficCircleOut_T;

fragment TrafficCircleOut_T fsTrafficCircle(const TrafficCircleFragment_T in [[stage_in]],
                                            constant Uniforms_T & uniforms [[buffer(0)]])
{
  constexpr float kAntialiasingThreshold = 0.92;
  
  TrafficCircleOut_T out;
  
  float4 color = in.color;
  
  float smallRadius = in.radius.z * kAntialiasingThreshold;
  float stepValue = smoothstep(smallRadius * smallRadius, in.radius.z * in.radius.z,
                               in.radius.x * in.radius.x + in.radius.y * in.radius.y);
  color.a = uniforms.u_opacity * (1.0 - stepValue);
  if (color.a < 0.001)
    out.depth = 1.0;
  else
    out.depth = in.position.z * in.position.w;
  out.color = color;
  return out;
}
