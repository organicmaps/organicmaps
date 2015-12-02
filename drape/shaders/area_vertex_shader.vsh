attribute vec3 a_position;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main(void)
{
  gl_Position = vec4(a_position, 1) * modelView * projection;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
}
