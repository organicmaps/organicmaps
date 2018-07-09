attribute vec3 a_position;
attribute vec2 a_colorTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

varying vec2 v_colorTexCoord;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  v_colorTexCoord = a_colorTexCoord;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
