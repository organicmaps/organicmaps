attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform float u_arrowHalfWidth;

varying vec2 v_colorTexCoords;

void main(void)
{
  float normalLen = length(a_normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (normalLen != 0.0)
  {
    vec2 norm = a_normal * u_arrowHalfWidth;
    float actualHalfWidth = length(norm);

    vec4 glbShiftPos = vec4(a_position.xy + norm, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * actualHalfWidth;
  }

  v_colorTexCoords = a_colorTexCoords;

  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  gl_Position = pos;
}
