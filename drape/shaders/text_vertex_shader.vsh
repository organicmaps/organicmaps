attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoord;
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

void main()
{
  // Here we intentionally decrease precision of 'pos' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pos = vec4(a_position.xyz, 1) * modelView;
  highp vec4 shiftedPos = vec4(a_normal, Zero, Zero) + pos;
  shiftedPos = shiftedPos * projection;
  float w = shiftedPos.w;
  shiftedPos.xyw = (pivotTransform * vec4(shiftedPos.xy, 0.0, w)).xyw;
  shiftedPos.z *= shiftedPos.w / w;
  gl_Position = shiftedPos;
  
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoord = a_colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
