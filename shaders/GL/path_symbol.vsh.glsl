attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

varying vec2 v_colorTexCoords;

void main()
{
  vec4 pos = vec4(a_position.xyz, 1) * u_modelView;

  float normalLen = length(a_normal);
  vec4 n = vec4(a_position.xy + a_normal * kShapeCoordScalar, 0.0, 0.0) * u_modelView;
  vec4 norm = vec4(0.0, 0.0, 0.0, 0.0);
  if (dot(n, n) != 0.0)
    norm = normalize(n) * normalLen;

  vec4 shiftedPos = norm + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  v_colorTexCoords = a_colorTexCoords;
}
