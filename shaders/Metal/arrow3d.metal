#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float4x4 u_transform;
  float4 u_color;
} Uniforms_T;

typedef struct
{
  float4 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
} Vertex_T;

typedef struct
{
  float4 position [[position]];
  float2 intensity;
} Fragment_T;

typedef struct
{
  float4 a_position [[attribute(0)]];
} VertexShadow_T;

typedef struct
{
  float4 position [[position]];
  float intensity;
} FragmentShadow_T;

vertex Fragment_T vsArrow3d(const Vertex_T in [[stage_in]],
                            constant Uniforms_T & uniforms [[buffer(2)]])
{
  constexpr float3 kLightDir = float3(0.316, 0.0, 0.948);

  Fragment_T out;
  out.position = uniforms.u_transform * float4(in.a_position.xyz, 1.0);
  out.intensity = float2(max(0.0, -dot(kLightDir, in.a_normal)), in.a_position.w);
  return out;
}

fragment float4 fsArrow3d(const Fragment_T in [[stage_in]],
                          constant Uniforms_T & uniforms [[buffer(0)]])
{
  float alpha = smoothstep(0.8, 1.0, in.intensity.y);
  return float4((in.intensity.x * 0.5 + 0.5) * uniforms.u_color.rgb, uniforms.u_color.a * alpha);
}

fragment float4 fsArrow3dOutline(const FragmentShadow_T in [[stage_in]],
                                 constant Uniforms_T & uniforms [[buffer(0)]])
{
  float alpha = smoothstep(0.7, 1.0, in.intensity);
  return float4(uniforms.u_color.rgb, uniforms.u_color.a * alpha);
}

vertex FragmentShadow_T vsArrow3dShadow(const VertexShadow_T in [[stage_in]],
                                        constant Uniforms_T & uniforms [[buffer(1)]])
{
  FragmentShadow_T out;
  out.position = uniforms.u_transform * float4(in.a_position.xy, 0.0, 1.0);
  out.intensity = in.a_position.w;
  return out;
}

fragment float4 fsArrow3dShadow(const FragmentShadow_T in [[stage_in]],
                                constant Uniforms_T & uniforms [[buffer(0)]])
{
  return float4(uniforms.u_color.rgb, uniforms.u_color.a * in.intensity);
}
