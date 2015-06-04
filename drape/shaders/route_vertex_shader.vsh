attribute vec3 a_position;
attribute vec2 a_normal;

uniform mat4 modelView;
uniform mat4 projection;

uniform float u_halfWidth;

void main(void)
{
  float halfWidth = length(a_normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (u_halfWidth != 0.0 && halfWidth != 0.0)
  {
    vec2 norm = a_normal * u_halfWidth;
    float actualHalfWidth = length(norm);

    vec4 glbShiftPos = vec4(a_position.xy + norm, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * actualHalfWidth;
  }

  gl_Position = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
}
