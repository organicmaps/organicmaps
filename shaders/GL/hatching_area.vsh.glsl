in vec3 a_position;
in vec2 a_colorTexCoords;
in vec2 a_maskTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
out LOW_P vec4 v_color;
#else
out vec2 v_colorTexCoords;
#endif
out vec2 v_maskTexCoords;

void main()
{
  vec4 pos = vec4(a_position, 1) * u_modelView * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
  v_maskTexCoords = a_maskTexCoords;
}
