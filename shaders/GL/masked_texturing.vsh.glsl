in vec4 a_position;
in vec2 a_normal;
in vec2 a_colorTexCoords;
in vec2 a_maskTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

out vec2 v_colorTexCoords;
out vec2 v_maskTexCoords;

void main()
{
  vec4 pos = vec4(a_position.xyz, 1) * u_modelView;
  vec4 shiftedPos = vec4(a_normal, 0, 0) + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  v_colorTexCoords = a_colorTexCoords;
  v_maskTexCoords = a_maskTexCoords;
}
