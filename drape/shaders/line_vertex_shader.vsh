attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoord;

uniform mat4 modelView;
uniform mat4 projection;

varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying vec2 v_halfLength;

void main(void)
{
  vec2 normal = a_normal.xy;
  float halfWidth = length(normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (halfWidth != 0.0)
  {
    vec4 glbShiftPos = vec4(a_position.xy + normal, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * halfWidth;
  }

  v_colorTexCoord = a_colorTexCoord;
  v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z));
  gl_Position = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
}
