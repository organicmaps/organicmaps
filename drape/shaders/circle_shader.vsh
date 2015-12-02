attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;

varying vec3 v_radius;
#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main(void)
{
  gl_Position = (vec4(a_normal.xy, 0, 0) + vec4(a_position, 1) * modelView) * projection;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
  v_radius = a_normal;
}
