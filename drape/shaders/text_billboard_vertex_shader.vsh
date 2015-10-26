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
  // Here we intentionally decrease precision of 'pivot' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pivot = a_position * modelView;
  vec4 offset = vec4(a_normal, Zero, Zero);
    
  pivot = pivot * projection;
  offset = offset * projection;

  gl_Position = pivotTransform * vec4(pivot.xy, Zero, One);
    
  vec4 scale = pivotTransform * vec4(One, -One, Zero, One);
  gl_Position = gl_Position + vec4(offset.xy * gl_Position.w / scale.w * scale.x, Zero, Zero);

#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoord);
#else
  v_colorTexCoord = a_colorTexCoord;
#endif
  v_maskTexCoord = a_maskTexCoord;
}
