#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  packed_float2 a_position;
  packed_float2 a_texCoords;
} Vertex_T;

typedef struct
{
  float4 position [[position]];
  float2 texCoords;
} Fragment_T;

typedef struct
{
  float u_opacity;
} Uniforms_T;

vertex Fragment_T vsScreenQuad(device const Vertex_T * vertices [[buffer(0)]],
                               uint vid [[vertex_id]])
{
  Vertex_T const in = vertices[vid];
  Fragment_T out;
  out.position = float4(in.a_position, 0.0, 1.0);
  out.texCoords = in.a_texCoords;
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
