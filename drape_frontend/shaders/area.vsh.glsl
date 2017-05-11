attribute vec3 a_position;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main(void)
{
  vec4 pos = vec4(a_position, 1) * modelView * projection;
  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  gl_Position = pos;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
}
