attribute vec3 a_position;
attribute vec2 a_colorTexCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoord;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  v_colorTexCoord = a_colorTexCoord;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
}
