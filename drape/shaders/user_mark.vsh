attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;
attribute float a_animate;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float u_interpolationT;

varying vec2 v_colorTexCoords;

void main(void)
{
  vec2 normal = a_normal;
  if (a_animate > 0.0)
    normal = u_interpolationT * normal;

  vec4 pos = (vec4(normal, 0, 0) + vec4(a_position, 1) * modelView) * projection;
  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  gl_Position = pos;
  v_colorTexCoords = a_colorTexCoords;
}
