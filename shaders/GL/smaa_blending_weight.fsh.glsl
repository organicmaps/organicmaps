// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa
layout (location = 0) in vec4 v_coords;
layout (location = 1) in vec4 v_offset0;
layout (location = 2) in vec4 v_offset1;
layout (location = 3) in vec4 v_offset2;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  vec4 u_framebufferMetrics;
};

layout (binding = 1) uniform sampler2D u_colorTex;
layout (binding = 2) uniform sampler2D u_smaaArea;
layout (binding = 3) uniform sampler2D u_smaaSearch;

#define SMAA_SEARCHTEX_SIZE vec2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE vec2(64.0, 16.0)
#define SMAA_AREATEX_MAX_DISTANCE 16.0
#define SMAA_AREATEX_PIXEL_SIZE (vec2(1.0 / 256.0, 1.0 / 1024.0))
#define SMAALoopBegin(condition) while (condition) {
#define SMAALoopEnd }
#define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) textureLodOffset(tex, coord, 0.0, offset)
#define SMAARound(v) round((v))
#define SMAAOffset(x,y) ivec2(x,y)

const vec2 kAreaTexMaxDistance = vec2(SMAA_AREATEX_MAX_DISTANCE, SMAA_AREATEX_MAX_DISTANCE);
const float kActivationThreshold = 0.8281;

float SMAASearchLength(vec2 e, float offset)
{
  // The texture is flipped vertically, with left and right cases taking half
  // of the space horizontally.
  vec2 scale = SMAA_SEARCHTEX_SIZE * vec2(0.5, -1.0);
  vec2 bias = SMAA_SEARCHTEX_SIZE * vec2(offset, 1.0);
  // Scale and bias to access texel centers.
  scale += vec2(-1.0,  1.0);
  bias += vec2( 0.5, -0.5);
  // Convert from pixel coordinates to texcoords.
  // (We use SMAA_SEARCHTEX_PACKED_SIZE because the texture is cropped).
  scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
  bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
  // Lookup the search texture.
  return SMAASampleLevelZero(u_smaaSearch, scale * e + bias).r;
}

float SMAASearchXLeft(vec2 texcoord, float end)
{
  vec2 e = vec2(0.0, 1.0);
  SMAALoopBegin(texcoord.x > end && e.g > kActivationThreshold && e.r == 0.0)
    e = SMAASampleLevelZero(u_colorTex, texcoord).rg;
    texcoord = vec2(-2.0, 0.0) * u_framebufferMetrics.xy + texcoord;
  SMAALoopEnd
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.0);
  return u_framebufferMetrics.x * offset + texcoord.x;
}

float SMAASearchXRight(vec2 texcoord, float end)
{
  vec2 e = vec2(0.0, 1.0);
  SMAALoopBegin(texcoord.x < end && e.g > kActivationThreshold && e.r == 0.0)
    e = SMAASampleLevelZero(u_colorTex, texcoord).rg;
    texcoord = vec2(2.0, 0.0) * u_framebufferMetrics.xy + texcoord;
  SMAALoopEnd
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e, 0.5);
  return -u_framebufferMetrics.x * offset + texcoord.x;
}

float SMAASearchYUp(vec2 texcoord, float end)
{
  vec2 e = vec2(1.0, 0.0);
  SMAALoopBegin(texcoord.y > end && e.r > kActivationThreshold && e.g == 0.0)
    e = SMAASampleLevelZero(u_colorTex, texcoord).rg;
    texcoord = vec2(0.0, -2.0) * u_framebufferMetrics.xy + texcoord;
  SMAALoopEnd
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.0);
  return u_framebufferMetrics.y * offset + texcoord.y;
}

float SMAASearchYDown(vec2 texcoord, float end)
{
  vec2 e = vec2(1.0, 0.0);
  SMAALoopBegin(texcoord.y < end && e.r > kActivationThreshold && e.g == 0.0)
    e = SMAASampleLevelZero(u_colorTex, texcoord).rg;
    texcoord = vec2(0.0, 2.0) * u_framebufferMetrics.xy + texcoord;
  SMAALoopEnd
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(e.gr, 0.5);
  return -u_framebufferMetrics.y * offset + texcoord.y;
}

// Here, we have the distance and both crossing edges. So, what are the areas
// at each side of current edge?
vec2 SMAAArea(vec2 dist, float e1, float e2)
{
  // Rounding prevents precision errors of bilinear filtering.
  vec2 texcoord = kAreaTexMaxDistance * SMAARound(4.0 * vec2(e1, e2)) + dist;
  // We do a scale and bias for mapping to texel space.
  texcoord = SMAA_AREATEX_PIXEL_SIZE * (texcoord + 0.5);
  return SMAASampleLevelZero(u_smaaArea, texcoord).rg;
}

void main()
{
  vec4 weights = vec4(0.0, 0.0, 0.0, 0.0);
  vec2 e = texture(u_colorTex, v_coords.xy).rg;
  if (e.g > 0.0) // Edge at north
  {
    vec2 d;
    // Find the distance to the left.
    vec3 coords;
    coords.x = SMAASearchXLeft(v_offset0.xy, v_offset2.x);
    coords.y = v_offset1.y;
    d.x = coords.x;
    // Now fetch the left crossing edges, two at a time using bilinear
    // filtering. Sampling at -0.25 enables to discern what value each edge has.
    float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).r;
    // Find the distance to the right.
    coords.z = SMAASearchXRight(v_offset0.zw, v_offset2.y);
    d.y = coords.z;
    // We want the distances to be in pixel units (doing this here allow to
    // better interleave arithmetic and memory accesses).
    vec2 zz = u_framebufferMetrics.zz;
    d = abs(SMAARound(zz * d - v_coords.zz));
    // SMAAArea below needs a sqrt, as the areas texture is compressed
    // quadratically.
    vec2 sqrt_d = sqrt(d);
    // Fetch the right crossing edges.
    float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.zy, SMAAOffset(1, 0)).r;
    // Here we know how this pattern looks like, now it is time for getting
    // the actual area.
    weights.rg = SMAAArea(sqrt_d, e1, e2);
  }
  if (e.r > 0.0) // Edge at west
  {
    vec2 d;
    // Find the distance to the top.
    vec3 coords;
    coords.y = SMAASearchYUp(v_offset1.xy, v_offset2.z);
    coords.x = v_offset0.x;
    d.x = coords.y;
    // Fetch the top crossing edges.
    float e1 = SMAASampleLevelZero(u_colorTex, coords.xy).g;
    // Find the distance to the bottom.
    coords.z = SMAASearchYDown(v_offset1.zw, v_offset2.w);
    d.y = coords.z;
    // We want the distances to be in pixel units.
    vec2 ww = u_framebufferMetrics.ww;
    d = abs(SMAARound(ww * d - v_coords.ww));
    // SMAAArea below needs a sqrt, as the areas texture is compressed
    // quadratically.
    vec2 sqrt_d = sqrt(d);
    // Fetch the bottom crossing edges.
    float e2 = SMAASampleLevelZeroOffset(u_colorTex, coords.xz, SMAAOffset(0, 1)).g;
    // Get the area for this direction.
    weights.ba = SMAAArea(sqrt_d, e1, e2);
  }
  v_FragColor = weights;
}
