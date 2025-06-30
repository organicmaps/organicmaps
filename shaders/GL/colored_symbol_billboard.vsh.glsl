in vec3 a_position;
in vec4 a_normal;
in vec4 a_colorTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

out vec4 v_normal;
#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
out LOW_P vec4 v_color;
#else
out vec2 v_colorTexCoords;
#endif

void main()
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * u_modelView;
  vec4 offset = vec4(a_normal.xy + a_colorTexCoords.zw, 0.0, 0.0) * u_projection;
  gl_Position = applyBillboardPivotTransform(pivot * u_projection, u_pivotTransform, 0.0, offset.xy);

#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords.xy);
#else
  v_colorTexCoords = a_colorTexCoords.xy;
#endif
  v_normal = a_normal;
}
