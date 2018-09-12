#pragma once

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
