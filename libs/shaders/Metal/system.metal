#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float2 a_position [[attribute(0)]];
} Vertex_T;

typedef struct
{
  float4 position [[position]];
} Fragment_T;

typedef struct
{
  float4 u_color;
} Uniforms_T;

typedef struct
{
  float4 color [[color(0)]];
  float depth [[depth(any)]];
} Fragment_Output;

typedef struct
{
  float depth [[depth(any)]];
} Fragment_DepthOutput;

vertex Fragment_T vsCleaner(const Vertex_T in [[stage_in]])
{
  Fragment_T out;
  out.position = float4(in.a_position, 0.0, 1.0);
  return out;
}

fragment float4 fsClearColor(const Fragment_T in [[stage_in]],
                             constant Uniforms_T & uniforms [[buffer(0)]])
{
  return uniforms.u_color;
}

fragment Fragment_DepthOutput fsClearDepth(const Fragment_T in [[stage_in]])
{
  Fragment_DepthOutput out;
  out.depth = 1.0;
  return out;
}

fragment Fragment_Output fsClearColorAndDepth(const Fragment_T in [[stage_in]],
                                              constant Uniforms_T & uniforms [[buffer(0)]])
{
  Fragment_Output out;
  out.color = uniforms.u_color;
  out.depth = 1.0;
  return out;
}
