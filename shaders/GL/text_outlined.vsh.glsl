attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_outlineColorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;
uniform float u_isOutlinePass;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying LOW_P vec4 v_color;
#else
varying vec2 v_colorTexCoord;
#endif

varying vec2 v_maskTexCoord;

const float BaseDepthShift = -10.0;

void main()
{
  float isOutline = step(0.5, u_isOutlinePass);
  float notOutline = 1.0 - isOutline;
  float depthShift = BaseDepthShift * isOutline;

  vec4 pos = (vec4(a_position.xyz, 1) + vec4(0.0, 0.0, depthShift, 0.0)) * u_modelView;
  vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos;
  gl_Position = applyPivotTransform(shiftedPos * u_projection, u_pivotTransform, 0.0);
  vec2 colorTexCoord = a_colorTexCoord * notOutline + a_outlineColorTexCoord * isOutline;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, colorTexCoord);
#else
  v_colorTexCoord = colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
