// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

attribute vec2 a_pos;
attribute vec2 a_tcoord;

uniform vec4 u_framebufferMetrics;

varying vec4 v_coords;
varying vec4 v_offset0;
varying vec4 v_offset1;
varying vec4 v_offset2;

// SMAA_MAX_SEARCH_STEPS specifies the maximum steps performed in the
// horizontal/vertical pattern searches, at each side of the pixel.
#define SMAA_MAX_SEARCH_STEPS 8.0
const vec4 kMaxSearchSteps = vec4(-2.0 * SMAA_MAX_SEARCH_STEPS, 2.0 * SMAA_MAX_SEARCH_STEPS,
                                  -2.0 * SMAA_MAX_SEARCH_STEPS, 2.0  * SMAA_MAX_SEARCH_STEPS);

void main()
{
  v_coords = vec4(a_tcoord, a_tcoord * u_framebufferMetrics.zw);
  // We will use these offsets for the searches.
  v_offset0 = u_framebufferMetrics.xyxy * vec4(-0.25, -0.125, 1.25, -0.125) + a_tcoord.xyxy;
  v_offset1 = u_framebufferMetrics.xyxy * vec4(-0.125, -0.25, -0.125, 1.25) + a_tcoord.xyxy;
  // And these for the searches, they indicate the ends of the loops.
  v_offset2 = u_framebufferMetrics.xxyy * kMaxSearchSteps + vec4(v_offset0.xz, v_offset1.yw);
  gl_Position = vec4(a_pos, 0.0, 1.0);
}
