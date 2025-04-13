attribute vec3 a_position;
attribute vec3 a_normalAndAnimateOrZ;
attribute vec4 a_texCoords;
attribute vec4 a_color;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;
uniform float u_interpolation;

varying vec4 v_texCoords;
varying vec4 v_maskColor;

void main()
{
  vec2 normal = a_normalAndAnimateOrZ.xy;
  if (a_normalAndAnimateOrZ.z > 0.0)
    normal = u_interpolation * normal;

  vec4 p = vec4(a_position, 1.0) * u_modelView;
  vec4 pos = vec4(normal, 0.0, 0.0) + p;
  vec4 projectedPivot = p * u_projection;
  gl_Position = applyPivotTransform(pos * u_projection, u_pivotTransform, 0.0);
  float newZ = projectedPivot.y / projectedPivot.w * 0.5 + 0.5;
  gl_Position.z = abs(a_normalAndAnimateOrZ.z) * newZ + (1.0 - abs(a_normalAndAnimateOrZ.z)) * gl_Position.z;
  v_texCoords = a_texCoords;
  v_maskColor = a_color;
}
