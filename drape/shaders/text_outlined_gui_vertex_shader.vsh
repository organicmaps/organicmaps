attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_outlineColorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform float u_isOutlinePass;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoord;
#endif

varying vec2 v_maskTexCoord;

const float Zero = 0.0;
const float One = 1.0;
const float BaseDepthShift = -10.0;

void main()
{
  float isOutline = step(0.5, u_isOutlinePass);
  float notOutline = One - isOutline;
  float depthShift = BaseDepthShift * isOutline;
  
  // Here we intentionally decrease precision of 'pos' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pos = (vec4(a_position.xyz, 1.0) + vec4(Zero, Zero, depthShift, Zero)) * modelView;
  highp vec4 shiftedPos = vec4(a_normal, Zero, Zero) + pos;
  gl_Position = shiftedPos * projection;
  vec2 colorTexCoord = a_colorTexCoord * notOutline + a_outlineColorTexCoord * isOutline;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, colorTexCoord);
#else
  v_colorTexCoord = colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
