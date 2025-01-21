in vec3 a_position;
in vec3 a_normal;
in vec2 a_colorTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
out LOW_P vec4 v_color;
#else
out vec2 v_colorTexCoord;
#endif

//out vec2 v_halfLength;

void main()
{
  vec2 normal = a_normal.xy;
  float halfWidth = length(normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (halfWidth != 0.0)
  {
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal,
                                                    u_modelView, halfWidth);
  }

#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoord = a_colorTexCoord;
#endif
  //v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z));
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
