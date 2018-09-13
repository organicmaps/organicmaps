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
