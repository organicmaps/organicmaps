#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

float4 ApplyPivotTransform(float4 pivot, float4x4 pivotTransform, float pivotRealZ)
{
  float4 transformedPivot = pivot;
  float w = transformedPivot.w;
  transformedPivot.xyw = (pivotTransform * float4(transformedPivot.xy, pivotRealZ, w)).xyw;
  transformedPivot.z *= transformedPivot.w / w;
  return transformedPivot;
}

float2 CalcLineTransformedAxisPos(float2 originalAxisPos, float2 shiftedPos, float4x4 modelView, float halfWidth)
{
  float2 p = (float4(shiftedPos, 0.0, 1.0) * modelView).xy;
  float2 d = p - originalAxisPos;
  if (dot(d, d) != 0.0)
    return originalAxisPos + normalize(d) * halfWidth;
  else
    return originalAxisPos;
}
