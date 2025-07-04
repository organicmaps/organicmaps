layout (location = 0) in vec4 a_position;
layout (location = 1) in vec2 a_normal;
layout (location = 2) in vec2 a_colorTexCoords;

layout (location = 0) out vec2 v_colorTexCoords;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_routeParams;
  vec4 u_color;
  vec4 u_maskColor;
  vec4 u_outlineColor;
  vec4 u_fakeColor;
  vec4 u_fakeOutlineColor;
  vec2 u_fakeBorders;
  vec2 u_pattern;
  vec2 u_angleCosSin;
  float u_arrowHalfWidth;
  float u_opacity;
};

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
