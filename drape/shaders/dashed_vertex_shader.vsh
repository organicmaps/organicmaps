attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;

void main(void)
{
  float halfWidth = length(a_normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (halfWidth != 0.0)
  {
    vec4 glbShiftPos = vec4(a_position.xy + a_normal, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * halfWidth;
  }

  v_colorTexCoord = a_colorTexCoord;
  v_maskTexCoord = a_maskTexCoord;
  gl_Position = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
}
