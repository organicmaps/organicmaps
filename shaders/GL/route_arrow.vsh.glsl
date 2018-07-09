attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform float u_arrowHalfWidth;

varying vec2 v_colorTexCoords;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (dot(a_normal, a_normal) != 0.0)
  {
    vec2 norm = a_normal * u_arrowHalfWidth;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }

  v_colorTexCoords = a_colorTexCoords;

  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
