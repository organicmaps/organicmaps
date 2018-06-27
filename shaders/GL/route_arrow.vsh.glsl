attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

uniform float u_arrowHalfWidth;

varying vec2 v_colorTexCoords;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (dot(a_normal, a_normal) != 0.0)
  {
    vec2 norm = a_normal * u_arrowHalfWidth;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    modelView, length(norm));
  }

  v_colorTexCoords = a_colorTexCoords;

  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
}
