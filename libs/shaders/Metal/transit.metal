#include <metal_stdlib>
#include <simd/simd.h>
#include "shaders_lib.h"
using namespace metal;

typedef struct
{
  float4x4 u_modelView;
  float4x4 u_projection;
  float4x4 u_pivotTransform;
  packed_float3 u_params;
  float u_dummy1;
  float u_lineHalfWidth;
  float u_maxRadius;
} Uniforms_T;

typedef struct
{
  float3 a_position [[attribute(0)]];
  float4 a_normal [[attribute(1)]];
  float4 a_color [[attribute(2)]];
} TransitVertex_T;

// Transit

typedef struct
{
  float4 position [[position]];
  float4 color;
} TransitFragment_T;

vertex TransitFragment_T vsTransit(const TransitVertex_T in [[stage_in]],
                                   constant Uniforms_T & uniforms [[buffer(1)]])
{
  TransitFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    float2 norm = normal * uniforms.u_lineHalfWidth;
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + norm,
                                                    uniforms.u_modelView, length(norm));
  }
  out.color = in.a_color;
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  return out;
}

fragment float4 fsTransit(const TransitFragment_T in [[stage_in]])
{
  return in.color;
}

// Transit marker

typedef struct
{
  float4 position [[position]];
  float4 offsets;
  float4 color;
} TransitMarkerFragment_T;

vertex TransitMarkerFragment_T vsTransitMarker(const TransitVertex_T in [[stage_in]],
                                               constant Uniforms_T & uniforms [[buffer(1)]])
{
  TransitMarkerFragment_T out;
  
  float4 pos = float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView;
  float3 params = uniforms.u_params;
  float2 normal = float2(in.a_normal.x * params.x - in.a_normal.y * params.y,
                         in.a_normal.x * params.y + in.a_normal.y * params.x);
  float2 shiftedPos = normal * params.z + pos.xy;
  pos = float4(shiftedPos, in.a_position.z, 1.0) * uniforms.u_projection;
  
  float2 offsets = abs(in.a_normal.zw);
  out.offsets = float4(in.a_normal.zw, offsets - 1.0);
  out.color = in.a_color;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  return out;
}

fragment float4 fsTransitMarker(const TransitMarkerFragment_T in [[stage_in]])
{
  float4 finalColor = in.color;
  float2 radius;
  radius.x = max(0.0, abs(in.offsets.x) - in.offsets.z);
  radius.y = max(0.0, abs(in.offsets.y) - in.offsets.w);
  
  float maxRadius = 1.0;
  float aaRadius = 0.9;
  float stepValue = 1.0 - smoothstep(aaRadius * aaRadius, maxRadius * maxRadius, dot(radius.xy, radius.xy));
  finalColor.a *= stepValue;

  return finalColor;
}

// Transit circle

typedef struct
{
  float4 position [[position]];
  float3 radius;
  float4 color;
} TransitCircleFragment_T;

vertex TransitCircleFragment_T vsTransitCircle(const TransitVertex_T in [[stage_in]],
                                               constant Uniforms_T & uniforms [[buffer(1)]])
{
  TransitCircleFragment_T out;
  
  float2 normal = in.a_normal.xy;
  float2 transformedAxisPos = (float4(in.a_position.xy, 0.0, 1.0) * uniforms.u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    float2 norm = normal * uniforms.u_lineHalfWidth;
    transformedAxisPos = CalcLineTransformedAxisPos(transformedAxisPos, in.a_position.xy + norm,
                                                    uniforms.u_modelView, length(norm));
  }
  transformedAxisPos += in.a_normal.zw * uniforms.u_lineHalfWidth;
  float4 pos = float4(transformedAxisPos, in.a_position.z, 1.0) * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  out.color = in.a_color;
  out.radius = float3(in.a_normal.zw, uniforms.u_maxRadius) * uniforms.u_lineHalfWidth;
  return out;
}

typedef struct
{
  float4 color [[color(0)]];
  float depth [[depth(any)]];
} TransitCircleOut_T;

fragment TransitCircleOut_T fsTransitCircle(const TransitCircleFragment_T in [[stage_in]])
{
  constexpr float kAntialiasingPixelsCount = 2.5;
 
  TransitCircleOut_T out;
  
  float4 finalColor = in.color;
  
  float smallRadius = in.radius.z - kAntialiasingPixelsCount;
  float stepValue = smoothstep(smallRadius * smallRadius, in.radius.z * in.radius.z,
                               dot(in.radius.xy, in.radius.xy));
  finalColor.a = finalColor.a * (1.0 - stepValue);
  if (finalColor.a < 0.001)
    out.depth = 1.0;
  else
    out.depth = in.position.z * in.position.w;
  
  out.color = finalColor;
  
  return out;
}
