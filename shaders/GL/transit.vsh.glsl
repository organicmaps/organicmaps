attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_color;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform float u_lineHalfWidth;

varying vec4 v_color;

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    vec2 norm = normal * u_lineHalfWidth;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    modelView, length(norm));
  }
  v_color = a_color;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
}
