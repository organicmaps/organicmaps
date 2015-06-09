attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_length;

uniform mat4 modelView;
uniform mat4 projection;

uniform vec2 u_halfWidth;

varying float v_length;

void main(void)
{
  float normalLen = length(a_normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  float len = a_length.x;
  if (u_halfWidth.x != 0.0 && normalLen != 0.0)
  {
    vec2 norm = a_normal * u_halfWidth.x;
    float actualHalfWidth = length(norm);

    vec4 glbShiftPos = vec4(a_position.xy + norm, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * actualHalfWidth;

    if (u_halfWidth.y != 0.0)
      len = a_length.x + a_length.y * u_halfWidth.y;
  }

  v_length = len;
  gl_Position = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
}
