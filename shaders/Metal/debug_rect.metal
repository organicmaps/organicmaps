#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  packed_float2 a_position;
} Vertex_T;

typedef struct
{
  float4 position [[position]];
} Fragment_T;

typedef struct
{
  float4 u_color;
} Uniforms_T;

vertex Fragment_T vsDebugRect(device const Vertex_T * vertices [[buffer(0)]],
                              uint vid [[vertex_id]])
{
  Fragment_T out;
  out.position = float4(vertices[vid].a_position, 0.0, 1.0);
  return out;
}

fragment float4 fsDebugRect(const Fragment_T in [[stage_in]],
                            constant Uniforms_T & uniforms [[buffer(0)]])
{
  return uniforms.u_color;
}
