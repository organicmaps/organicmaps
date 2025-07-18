#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float4x4 u_transform;
  float4x4 u_normalTransform;
  float4 u_color;
  float2 u_texCoordFlipping;
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

typedef struct
{
  float3 a_position [[attribute(0)]];
  float3 a_normal [[attribute(1)]];
  float2 a_texCoords [[attribute(2)]];
} VertexTextured_T;

typedef struct
{
  float4 position [[position]];
  float3 normal;
  float2 texCoords;
} FragmentTextured_T;

vertex Fragment_T vsArrow3d(const Vertex_T in [[stage_in]],
                            constant Uniforms_T & uniforms [[buffer(2)]])
{
  Fragment_T out;
  out.position = uniforms.u_transform * float4(in.a_position, 1.0);
  out.normal = normalize((uniforms.u_normalTransform * float4(in.a_normal, 0.0)).xyz);
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

vertex FragmentTextured_T vsArrow3dTextured(const VertexTextured_T in [[stage_in]],
                                            constant Uniforms_T & uniforms [[buffer(3)]])
{
  FragmentTextured_T out;
  out.position = uniforms.u_transform * float4(in.a_position, 1.0);
  out.normal = normalize((uniforms.u_normalTransform * float4(in.a_normal, 0.0)).xyz);
  out.texCoords = mix(in.a_texCoords, 1.0 - in.a_texCoords, uniforms.u_texCoordFlipping);
  return out;
}

fragment float4 fsArrow3dTextured(const FragmentTextured_T in [[stage_in]],
                                  constant Uniforms_T & uniforms [[buffer(0)]],
                                  texture2d<float> u_colorTex [[texture(0)]],
                                  sampler u_colorTexSampler [[sampler(0)]])
{
  constexpr float3 kLightDir = float3(0.316, 0.0, 0.948);
  float phongDiffuse = max(0.0, -dot(kLightDir, in.normal));
  float4 color = u_colorTex.sample(u_colorTexSampler, in.texCoords) * uniforms.u_color;
  return float4((phongDiffuse * 0.5 + 0.5) * color.rgb, color.a);
}
