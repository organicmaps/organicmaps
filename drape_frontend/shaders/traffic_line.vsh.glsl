attribute vec3 a_position;
attribute vec2 a_colorTexCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoord;

void main(void)
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  v_colorTexCoord = a_colorTexCoord;
  gl_Position = pos;
}
