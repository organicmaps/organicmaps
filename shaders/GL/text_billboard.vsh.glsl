attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;
uniform float u_isOutlinePass;
uniform float u_zScale;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying LOW_P vec4 v_color;
#else
varying vec2 v_colorTexCoord;
#endif

varying vec2 v_maskTexCoord;

void main()
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * u_modelView;
  vec4 offset = vec4(a_normal, 0.0, 0.0) * u_projection;
  gl_Position = applyBillboardPivotTransform(pivot * u_projection, u_pivotTransform,
                                             a_position.w * u_zScale, offset.xy);
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoord = a_colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
