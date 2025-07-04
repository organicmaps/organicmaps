// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec2 a_tcoord;

layout (location = 0) out vec2 v_colorTexCoords;
layout (location = 1) out vec4 v_offset0;
layout (location = 2) out vec4 v_offset1;
layout (location = 3) out vec4 v_offset2;

layout (binding = 0) uniform UBO
{
  vec4 u_framebufferMetrics;
};

void main()
{
  v_colorTexCoords = a_tcoord;
  v_offset0 = u_framebufferMetrics.xyxy * vec4(-1.0, 0.0, 0.0, -1.0) + a_tcoord.xyxy;
  v_offset1 = u_framebufferMetrics.xyxy * vec4( 1.0, 0.0, 0.0,  1.0) + a_tcoord.xyxy;
  v_offset2 = u_framebufferMetrics.xyxy * vec4(-2.0, 0.0, 0.0, -2.0) + a_tcoord.xyxy;
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
