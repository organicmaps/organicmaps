#include <metal_stdlib>
#include <simd/simd.h>
#include "shaders_lib.h"
using namespace metal;

#define TILE_BACKGROUND_MAX_COUNT 64

typedef struct
{
  float4 position [[position]];
  float3 texCoords;
} Fragment_T;

typedef struct
{
  float4 u_tileCoordsMinMax[TILE_BACKGROUND_MAX_COUNT];
  int u_textureIndex[TILE_BACKGROUND_MAX_COUNT];
  float4x4 u_modelView;
  float4x4 u_projection;
  float4x4 u_pivotTransform;
} Uniforms_T;

vertex Fragment_T vsTileBackground(uint vertexId [[vertex_id]],
                                   uint instanceId [[instance_id]],
                                   constant Uniforms_T & uniforms [[buffer(1)]])
{
  Fragment_T out;
  
  // Quad vertices: (0,0), (1,0), (0,1), (1,1) based on vertexId
  float2 quadVertex = float2(vertexId & 1, (vertexId >> 1) & 1);
  
  float4 tileCoordsMinMax = uniforms.u_tileCoordsMinMax[instanceId];
  float2 worldPos = mix(tileCoordsMinMax.xy, tileCoordsMinMax.zw, quadVertex);
  float4 pos = float4(worldPos, 0.0, 1.0) * uniforms.u_modelView * uniforms.u_projection;
  out.position = ApplyPivotTransform(pos, uniforms.u_pivotTransform, 0.0);
  
  out.texCoords = float3(quadVertex, float(uniforms.u_textureIndex[instanceId]));
  
  return out;
}

fragment float4 fsTileBackground(const Fragment_T in [[stage_in]],
                                 texture2d<float> u_colorTex [[texture(0)]],
                                 sampler u_colorTexSampler [[sampler(0)]])
{
  return u_colorTex.sample(u_colorTexSampler, in.texCoords.xy);
}
