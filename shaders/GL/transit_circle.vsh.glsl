attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_color;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform float u_lineHalfWidth;
uniform float u_maxRadius;

varying vec3 v_radius;
varying vec4 v_color;

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    vec2 norm = normal * u_lineHalfWidth;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }
  transformedAxisPos += a_normal.zw * u_lineHalfWidth;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  v_color = a_color;
  v_radius = vec3(a_normal.zw, u_maxRadius) * u_lineHalfWidth;
}
