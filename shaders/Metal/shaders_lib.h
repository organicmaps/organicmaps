#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Scale factor in shape's coordinates transformation from tile's coordinate
// system.
constant float kShapeCoordScalar = 1000.0;

float4 ApplyPivotTransform(float4 pivot, float4x4 pivotTransform, float pivotRealZ);

// This function calculates transformed position on an axis for line shaders family.
float2 CalcLineTransformedAxisPos(float2 originalAxisPos, float2 shiftedPos, float4x4 modelView, float halfWidth);
