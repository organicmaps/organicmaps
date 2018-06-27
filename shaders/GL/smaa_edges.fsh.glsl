// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

uniform sampler2D u_colorTex;

varying vec2 v_colorTexCoords;
varying vec4 v_offset0;
varying vec4 v_offset1;
varying vec4 v_offset2;

// SMAA_THRESHOLD specifies the threshold or sensitivity to edges.
// Lowering this value you will be able to detect more edges at the expense of
// performance.
// Range: [0, 0.5]
//   0.1 is a reasonable value, and allows to catch most visible edges.
//   0.05 is a rather overkill value, that allows to catch 'em all.
#define SMAA_THRESHOLD 0.05
const vec2 kThreshold = vec2(SMAA_THRESHOLD, SMAA_THRESHOLD);

// If there is an neighbor edge that has SMAA_LOCAL_CONTRAST_FACTOR times
// bigger contrast than current edge, current edge will be discarded.
// This allows to eliminate spurious crossing edges, and is based on the fact
// that, if there is too much contrast in a direction, that will hide
// perceptually contrast in the other neighbors.
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0

// Standard relative luminance weights.
// https://en.wikipedia.org/wiki/Relative_luminance
const vec3 kWeights = vec3(0.2126, 0.7152, 0.0722);

void main()
{
  // Calculate lumas.
  float L = dot(texture2D(u_colorTex, v_colorTexCoords).rgb, kWeights);
  float Lleft = dot(texture2D(u_colorTex, v_offset0.xy).rgb, kWeights);
  float Ltop = dot(texture2D(u_colorTex, v_offset0.zw).rgb, kWeights);

  // We do the usual threshold.
  vec4 delta;
  delta.xy = abs(L - vec2(Lleft, Ltop));
  vec2 edges = step(kThreshold, delta.xy);
  if (dot(edges, vec2(1.0, 1.0)) == 0.0)
      discard;

  // Calculate right and bottom deltas.
  float Lright = dot(texture2D(u_colorTex, v_offset1.xy).rgb, kWeights);
  float Lbottom  = dot(texture2D(u_colorTex, v_offset1.zw).rgb, kWeights);
  delta.zw = abs(L - vec2(Lright, Lbottom));

  // Calculate the maximum delta in the direct neighborhood.
  vec2 maxDelta = max(delta.xy, delta.zw);

  // Calculate left-left and top-top deltas.
  float Lleftleft = dot(texture2D(u_colorTex, v_offset2.xy).rgb, kWeights);
  float Ltoptop = dot(texture2D(u_colorTex, v_offset2.zw).rgb, kWeights);
  delta.zw = abs(vec2(Lleft, Ltop) - vec2(Lleftleft, Ltoptop));

  // Calculate the final maximum delta.
  maxDelta = max(maxDelta.xy, delta.zw);
  float finalDelta = max(maxDelta.x, maxDelta.y);

  // Local contrast adaptation
  edges *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);

  gl_FragColor = vec4(edges, 0.0, 1.0);
}
