attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec3 a_length;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform vec3 u_routeParams;

varying vec3 v_length;

void main(void)
{
  float normalLen = length(a_normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  vec2 len = vec2(a_length.x, a_length.z);
  if (u_routeParams.x != 0.0 && normalLen != 0.0)
  {
    vec2 norm = a_normal * u_routeParams.x;
    float actualHalfWidth = length(norm);

    vec4 glbShiftPos = vec4(a_position.xy + norm, 0.0, 1.0);
    vec2 shiftPos = (glbShiftPos * modelView).xy;
    transformedAxisPos = transformedAxisPos + normalize(shiftPos - transformedAxisPos) * actualHalfWidth;

    if (u_routeParams.y != 0.0)
      len = vec2(a_length.x + a_length.y * u_routeParams.y, a_length.z);
  }

  v_length = vec3(len, u_routeParams.z);
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  gl_Position = pos;
}
