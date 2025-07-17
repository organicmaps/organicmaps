attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;
attribute vec3 a_length;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform vec2 u_lineParams;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying LOW_P vec4 v_color;
#else
varying vec2 v_colorTexCoord;
#endif

varying float v_lengthY;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec2 len = vec2(a_length.x, a_length.z);
  if (dot(a_normal, a_normal) != 0.0)
  {
    vec2 norm = a_normal * u_lineParams.x;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
    if (u_lineParams.y != 0.0)
      len = vec2(a_length.x + a_length.y * u_lineParams.y, a_length.z);
  }

#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoord = a_colorTexCoords;
#endif
  v_lengthY = len.y;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
