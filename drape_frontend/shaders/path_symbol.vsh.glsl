attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;

const float kShapeCoordScalar = 1000.0;

void main(void)
{
  vec4 pos = vec4(a_position.xyz, 1) * modelView;

  float normalLen = length(a_normal);
  vec4 n = normalize(vec4(a_position.xy + a_normal * kShapeCoordScalar, 0, 0) * modelView);
  vec4 norm = n * normalLen;

  vec4 shiftedPos = norm + pos;
  shiftedPos = shiftedPos * projection;
  float w = shiftedPos.w;
  shiftedPos.xyw = (pivotTransform * vec4(shiftedPos.xy, 0.0, w)).xyw;
  shiftedPos.z *= shiftedPos.w / w;
  gl_Position = shiftedPos;
  v_colorTexCoords = a_colorTexCoords;
}
