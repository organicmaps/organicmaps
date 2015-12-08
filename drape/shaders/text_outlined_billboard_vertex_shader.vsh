attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec2 a_outlineColorTexCoord;
attribute vec2 a_maskTexCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
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
  lowp vec4 pivot = a_position * modelView;
  vec4 offset = vec4(a_normal, Zero, Zero) * projection;
  
  vec4 projectedPivot = pivot * projection;
  vec4 transformedPivot = pivotTransform * vec4(projectedPivot.xy, 0.0, 1.0);
  
  vec4 scale = pivotTransform * vec4(One, -One, Zero, One);
  gl_Position = transformedPivot + vec4(offset.xy * transformedPivot.w / scale.w * scale.x, Zero, Zero);
  
  vec2 colorTexCoord = a_colorTexCoord * notOutline + a_outlineColorTexCoord * isOutline;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, colorTexCoord);
#else
  v_colorTexCoord = colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
