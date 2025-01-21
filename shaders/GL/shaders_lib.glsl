// This is a library of functions which we are use in our shaders.
// Common (DO NOT modify this comment, it marks up block of common functions).

// Scale factor in shape's coordinates transformation from tile's coordinate
// system.
const float kShapeCoordScalar = 1000.0;

// VS (DO NOT modify this comment, it marks up block of vertex shader functions).

// This function applies a 2D->3D transformation matrix |pivotTransform| to |pivot|.
vec4 applyPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ)
{
  vec4 transformedPivot = pivot;
  float w = transformedPivot.w;
  transformedPivot.xyw = (pivotTransform * vec4(transformedPivot.xy, pivotRealZ, w)).xyw;
  transformedPivot.z *= transformedPivot.w / w;
#ifdef VULKAN
  transformedPivot.y = -transformedPivot.y;
  transformedPivot.z = (transformedPivot.z  + transformedPivot.w) / 2.0;
#endif
  return transformedPivot;
}

// This function applies a 2D->3D transformation matrix to billboards.
vec4 applyBillboardPivotTransform(vec4 pivot, mat4 pivotTransform, float pivotRealZ, vec2 offset)
{
  float logicZ = pivot.z / pivot.w;
  vec4 transformedPivot = pivotTransform * vec4(pivot.xy, pivotRealZ, pivot.w);
  vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0);
  vec4 position = vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset / scale.w * scale.x, 0.0, 0.0);
#ifdef VULKAN
  position.y = -position.y;
  position.z = (position.z  + position.w) / 2.0;
#endif
  return position;
}

// This function calculates transformed position on an axis for line shaders family.
vec2 calcLineTransformedAxisPos(vec2 originalAxisPos, vec2 shiftedPos, mat4 modelView, float halfWidth)
{
  vec2 p = (vec4(shiftedPos, 0.0, 1.0) * modelView).xy;
  vec2 d = p - originalAxisPos;
  if (dot(d, d) != 0.0)
    return originalAxisPos + normalize(d) * halfWidth;
  else
    return originalAxisPos;
}

// FS (DO NOT modify this comment, it marks up block of fragment shader functions).
