// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

in vec2 a_pos;
in vec2 a_tcoord;

uniform vec4 u_framebufferMetrics;

out vec2 v_colorTexCoords;
out vec4 v_offset0;
out vec4 v_offset1;
out vec4 v_offset2;

void main()
{
  v_colorTexCoords = a_tcoord;
  v_offset0 = u_framebufferMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + a_tcoord.xyxy;
  v_offset1 = u_framebufferMetrics.xyxy * vec4( 1.0, 0.0, 0.0,  1.0) + a_tcoord.xyxy;
  v_offset2 = u_framebufferMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + a_tcoord.xyxy;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}

