attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_accuracy;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec4 position = vec4(u_position, 1.0) * modelView;
  vec4 normal = vec4(normalize(a_normal) * u_accuracy, 0.0, 0.0);
  position = (position + normal) * projection;
  float w = position.w;
  position.xyw = (pivotTransform * position).xyw;
  position.z *= position.w / w;
  gl_Position = position;

  v_colorTexCoords = a_colorTexCoords;
}
