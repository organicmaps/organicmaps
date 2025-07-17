attribute vec2 a_position;
attribute vec2 a_colorTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;

varying vec2 v_colorTexCoords;

void main()
{
  gl_Position = vec4(a_position, 0, 1) * u_modelView * u_projection;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
  v_colorTexCoords = a_colorTexCoords;
}
