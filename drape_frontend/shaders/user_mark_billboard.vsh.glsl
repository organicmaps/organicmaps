attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;
attribute float a_animate;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float u_interpolationT;

varying vec2 v_colorTexCoords;

void main()
{
  vec2 normal = a_normal;
  if (a_animate > 0.0)
    normal = u_interpolationT * normal;

  vec4 pivot = vec4(a_position.xyz, 1.0) * modelView;
  vec4 offset = vec4(normal, 0.0, 0.0) * projection;
  vec4 projectedPivot = pivot * projection;
  gl_Position = applyBillboardPivotTransform(projectedPivot, pivotTransform, 0.0, offset.xy);
  gl_Position.z = projectedPivot.y / projectedPivot.w * 0.5 + 0.5;
  v_colorTexCoords = a_colorTexCoords;
}
