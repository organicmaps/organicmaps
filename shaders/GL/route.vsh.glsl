attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec3 a_length;
attribute vec4 a_color;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform vec4 u_routeParams;

varying vec3 v_length;
varying vec4 v_color;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  vec2 len = vec2(a_length.x, a_length.z);
  if (dot(a_normal, a_normal) != 0.0)
  {
    vec2 norm = a_normal * u_routeParams.x;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    modelView, length(norm));
    if (u_routeParams.y != 0.0)
      len = vec2(a_length.x + a_length.y * u_routeParams.y, a_length.z);
  }

  v_length = vec3(len, u_routeParams.z);
  v_color = a_color;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
}
