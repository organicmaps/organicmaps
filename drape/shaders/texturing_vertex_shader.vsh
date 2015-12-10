attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;

void main(void)
{
  // Here we intentionally decrease precision of 'pos' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pos = vec4(a_position.xyz, 1) * modelView;
  highp vec4 shiftedPos = vec4(a_normal, 0, 0) + pos;
  shiftedPos = shiftedPos * projection;
  float w = shiftedPos.w;
  shiftedPos.xyw = (pivotTransform * vec4(shiftedPos.xy, 0.0, w)).xyw;
  shiftedPos.z *= shiftedPos.w / w;
  gl_Position = shiftedPos;
  v_colorTexCoords = a_colorTexCoords;
}
