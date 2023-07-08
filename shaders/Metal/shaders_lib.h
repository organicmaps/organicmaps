#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

// Scale factor in shape's coordinates transformation from tile's coordinate
// system.
constant float kShapeCoordScalar = 1000.0;

// This function applies a 2D->3D transformation matrix.
float4 ApplyPivotTransform(float4 pivot, float4x4 pivotTransform, float pivotRealZ);

// This function applies a 2D->3D transformation matrix to billboards.
float4 ApplyBillboardPivotTransform(float4 pivot, float4x4 pivotTransform, float pivotRealZ, float2 offset);

// This function calculates transformed position on an axis for line shaders family.
float2 CalcLineTransformedAxisPos(float2 originalAxisPos, float2 shiftedPos, float4x4 modelView, float halfWidth);
