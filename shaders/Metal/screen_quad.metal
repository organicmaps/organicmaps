#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float2 a_position [[attribute(0)]];
  float2 a_texCoords [[attribute(1)]];
} Vertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} Fragment_T;

typedef struct
{
  float u_opacity;
  float u_invertV;
} Uniforms_T;

vertex Fragment_T vsScreenQuad(const Vertex_T in [[stage_in]],
                               constant Uniforms_T & uniforms [[buffer(1)]])
{
  Fragment_T out;
  out.position = float4(in.a_position, 0.0, 1.0);
  out.texCoords = mix(in.a_texCoords, float2(in.a_texCoords.x, 1.0 - in.a_texCoords.y), uniforms.u_invertV);
  return out;
}

fragment float4 fsScreenQuad(const Fragment_T in [[stage_in]],
                             constant Uniforms_T & uniforms [[buffer(0)]],
                             texture2d<float> u_colorTex [[texture(0)]],
                             sampler u_colorTexSampler [[sampler(0)]])
{
  float4 color = u_colorTex.sample(u_colorTexSampler, in.texCoords);
  color.a *= uniforms.u_opacity;
  return color;
}
