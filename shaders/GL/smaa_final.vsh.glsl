// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

attribute vec2 a_pos;
attribute vec2 a_tcoord;

uniform vec4 u_framebufferMetrics;

varying vec2 v_colorTexCoords;
varying vec4 v_offset;

void main()
{
  v_colorTexCoords = a_tcoord;
  v_offset = u_framebufferMetrics.xyxy * vec4(1.0, 0.0, 0.0, 1.0) + a_tcoord.xyxy;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
