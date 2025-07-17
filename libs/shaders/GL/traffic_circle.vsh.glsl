attribute vec4 a_position;
attribute vec4 a_normal;
attribute vec2 a_colorTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform vec3 u_lightArrowColor; // Here we store left sizes by road classes.
uniform vec3 u_darkArrowColor; // Here we store right sizes by road classes.

varying vec2 v_colorTexCoord;
varying vec3 v_radius;

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  int index = int(a_position.w);
  float leftSize = u_lightArrowColor[index];
  float rightSize = u_darkArrowColor[index];
  if (dot(normal, normal) != 0.0)
  {
    // offset by normal = rightVec * (rightSize - leftSize) / 2
    vec2 norm = normal * 0.5 * (rightSize - leftSize);
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }
  // radius = (leftSize + rightSize) / 2
  v_radius = vec3(a_normal.zw, 1.0) * 0.5 * (leftSize + rightSize);

  vec2 finalPos = transformedAxisPos + v_radius.xy;
  v_colorTexCoord = a_colorTexCoord;
  vec4 pos = vec4(finalPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
