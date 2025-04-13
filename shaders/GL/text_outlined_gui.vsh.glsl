attribute vec3 a_position;
attribute vec2 a_colorTexCoord;
attribute vec2 a_outlineColorTexCoord;
attribute vec2 a_normal;
attribute vec2 a_maskTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform float u_isOutlinePass;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying LOW_P vec4 v_color;
#else
varying vec2 v_colorTexCoord;
#endif

varying vec2 v_maskTexCoord;

const float kBaseDepthShift = -10.0;

void main()
{
  float isOutline = step(0.5, u_isOutlinePass);
  float depthShift = kBaseDepthShift * isOutline;
  
  vec4 pos = (vec4(a_position, 1.0) + vec4(0.0, 0.0, depthShift, 0.0)) * u_modelView;
  vec4 shiftedPos = vec4(a_normal, 0.0, 0.0) + pos;
  gl_Position = shiftedPos * u_projection;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
  vec2 colorTexCoord = mix(a_colorTexCoord, a_outlineColorTexCoord, isOutline);
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, colorTexCoord);
#else
  v_colorTexCoord = colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
