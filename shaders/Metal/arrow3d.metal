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
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
} Vertex_T;

typedef struct
{
  float4 position [[position]];
  float3 normal;
} Fragment_T;

typedef struct
{
  float3 a_position [[attribute(0)]];
  float2 a_texCoords [[attribute(1)]];
} VertexShadow_T;

typedef struct
{
  float4 position [[position]];
  float intensity;
} FragmentShadow_T;

vertex Fragment_T vsArrow3d(const Vertex_T in [[stage_in]],
                            constant Uniforms_T & uniforms [[buffer(2)]])
{
  Fragment_T out;
  out.position = uniforms.u_transform * float4(in.a_position, 1.0);
  out.normal = in.a_normal;
  return out;
}

fragment float4 fsArrow3d(const Fragment_T in [[stage_in]],
                          constant Uniforms_T & uniforms [[buffer(0)]])
{
  constexpr float3 kLightDir = float3(0.316, 0.0, 0.948);
  float phongDiffuse = max(0.0, -dot(kLightDir, in.normal));
  return float4((phongDiffuse * 0.5 + 0.5) * uniforms.u_color.rgb, uniforms.u_color.a);
}

vertex FragmentShadow_T vsArrow3dShadow(const VertexShadow_T in [[stage_in]],
                                        constant Uniforms_T & uniforms [[buffer(2)]])
{
  FragmentShadow_T out;
  out.position = uniforms.u_transform * float4(in.a_position, 1.0);
  out.intensity = in.a_texCoords.x;
  return out;
}

fragment float4 fsArrow3dShadow(const FragmentShadow_T in [[stage_in]],
                                constant Uniforms_T & uniforms [[buffer(0)]])
{
  return float4(uniforms.u_color.rgb, uniforms.u_color.a * in.intensity);
}

fragment float4 fsArrow3dOutline(const FragmentShadow_T in [[stage_in]],
                                 constant Uniforms_T & uniforms [[buffer(0)]])
{
  float alpha = smoothstep(0.7, 1.0, in.intensity);
  return float4(uniforms.u_color.rgb, uniforms.u_color.a * alpha);
}
