// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

typedef struct
{
  float4 u_framebufferMetrics;
} Uniforms_T;

typedef struct
{
  float2 a_pos [[attribute(0)]];
  float2 a_tcoord [[attribute(1)]];
} SmaaVertex_T;

// SmaaEdges

typedef struct
{
  float4 position [[position]];
  float4 offset0;
  float4 offset1;
  float4 offset2;
  float2 coords;
} SmaaEdgesFragment_T;

vertex SmaaEdgesFragment_T vsSmaaEdges(const SmaaVertex_T in [[stage_in]],
                                       constant Uniforms_T & uniforms [[buffer(1)]])
{
  SmaaEdgesFragment_T out;
  
  out.position = float4(in.a_pos, 0.0, 1.0);
  float2 tcoord = float2(in.a_tcoord.x, 1.0 - in.a_tcoord.y);
  out.coords = tcoord;
  float4 coords1 = uniforms.u_framebufferMetrics.xyxy;
  float4 coords2 = tcoord.xyxy;
  out.offset0 = coords1 * float4(-1.0, 0.0, 0.0, -1.0) + coords2;
  out.offset1 = coords1 * float4( 1.0, 0.0, 0.0,  1.0) + coords2;
  out.offset2 = coords1 * float4(-2.0, 0.0, 0.0, -2.0) + coords2;
  
  return out;
}

// SMAA_THRESHOLD specifies the threshold or sensitivity to edges.
// Lowering this value you will be able to detect more edges at the expense of
// performance.
// Range: [0, 0.5]
//   0.1 is a reasonable value, and allows to catch most visible edges.
//   0.05 is a rather overkill value, that allows to catch 'em all.
#define SMAA_THRESHOLD 0.05
constant float2 kThreshold = float2(SMAA_THRESHOLD, SMAA_THRESHOLD);

// If there is an neighbor edge that has SMAA_LOCAL_CONTRAST_FACTOR times
// bigger contrast than current edge, current edge will be discarded.
// This allows to eliminate spurious crossing edges, and is based on the fact
// that, if there is too much contrast in a direction, that will hide
// perceptually contrast in the other neighbors.
#define SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR 2.0

// Standard relative luminance weights.
// https://en.wikipedia.org/wiki/Relative_luminance
constant float3 kWeights = float3(0.2126, 0.7152, 0.0722);

fragment float4 fsSmaaEdges(const SmaaEdgesFragment_T in [[stage_in]],
                            texture2d<float> u_colorTex [[texture(0)]],
                            sampler u_colorTexSampler [[sampler(0)]])
{
  // Calculate lumas.
  float L = dot(u_colorTex.sample(u_colorTexSampler, in.coords).rgb, kWeights);
  float Lleft = dot(u_colorTex.sample(u_colorTexSampler, in.offset0.xy).rgb, kWeights);
  float Ltop = dot(u_colorTex.sample(u_colorTexSampler, in.offset0.zw).rgb, kWeights);
  
  // We do the usual threshold.
  float4 delta;
  delta.xy = abs(L - float2(Lleft, Ltop));
  float2 edges = step(kThreshold, delta.xy);
  if (dot(edges, float2(1.0, 1.0)) == 0.0)
    discard_fragment();
  
  // Calculate right and bottom deltas.
  float Lright = dot(u_colorTex.sample(u_colorTexSampler, in.offset1.xy).rgb, kWeights);
  float Lbottom  = dot(u_colorTex.sample(u_colorTexSampler, in.offset1.zw).rgb, kWeights);
  delta.zw = abs(L - float2(Lright, Lbottom));
  
  // Calculate the maximum delta in the direct neighborhood.
  float2 maxDelta = max(delta.xy, delta.zw);
  
  // Calculate left-left and top-top deltas.
  float Lleftleft = dot(u_colorTex.sample(u_colorTexSampler, in.offset2.xy).rgb, kWeights);
  float Ltoptop = dot(u_colorTex.sample(u_colorTexSampler, in.offset2.zw).rgb, kWeights);
  delta.zw = abs(float2(Lleft, Ltop) - float2(Lleftleft, Ltoptop));
  
  // Calculate the final maximum delta.
  maxDelta = max(maxDelta.xy, delta.zw);
  float finalDelta = max(maxDelta.x, maxDelta.y);
  
  // Local contrast adaptation.
  edges *= step(finalDelta, SMAA_LOCAL_CONTRAST_ADAPTATION_FACTOR * delta.xy);
  
  return float4(edges, 0.0, 1.0);
}

// SmaaBlendingWeight

typedef struct
{
  float4 position [[position]];
  float4 offset0;
  float4 offset1;
  float4 offset2;
  float4 coords;
} SmaaBlendingWeightFragment_T;

// SMAA_MAX_SEARCH_STEPS specifies the maximum steps performed in the
// horizontal/vertical pattern searches, at each side of the pixel.
#define SMAA_MAX_SEARCH_STEPS 8.0
constant float4 kMaxSearchSteps = float4(-2.0 * SMAA_MAX_SEARCH_STEPS, 2.0 * SMAA_MAX_SEARCH_STEPS,
                                         -2.0 * SMAA_MAX_SEARCH_STEPS, 2.0  * SMAA_MAX_SEARCH_STEPS);

vertex SmaaBlendingWeightFragment_T vsSmaaBlendingWeight(const SmaaVertex_T in [[stage_in]],
                                                         constant Uniforms_T & uniforms [[buffer(1)]])
{
  SmaaBlendingWeightFragment_T out;
  
  out.position = float4(in.a_pos, 0.0, 1.0);
  float2 tcoord = float2(in.a_tcoord.x, 1.0 - in.a_tcoord.y);
  out.coords = float4(tcoord, tcoord * uniforms.u_framebufferMetrics.zw);
  // We will use these offsets for the searches.
  float4 coords1 = uniforms.u_framebufferMetrics.xyxy;
  float4 coords2 = tcoord.xyxy;
  out.offset0 = coords1 * float4(-0.25, -0.125, 1.25, -0.125) + coords2;
  out.offset1 = coords1 * float4(-0.125, -0.25, -0.125, 1.25) + coords2;
  // And these for the searches, they indicate the ends of the loops.
  out.offset2 = uniforms.u_framebufferMetrics.xxyy * kMaxSearchSteps + float4(out.offset0.xz, out.offset1.yw);
  
  return out;
}

#define SMAA_SEARCHTEX_SIZE float2(66.0, 33.0)
#define SMAA_SEARCHTEX_PACKED_SIZE float2(64.0, 16.0)
#define SMAA_AREATEX_MAX_DISTANCE 16.0
#define SMAA_AREATEX_PIXEL_SIZE (float2(1.0 / 256.0, 1.0 / 1024.0))

constant float2 kAreaTexMaxDistance = float2(SMAA_AREATEX_MAX_DISTANCE, SMAA_AREATEX_MAX_DISTANCE);
constant float kActivationThreshold = 0.8281;

float SMAASearchLength(texture2d<float> u_smaaSearch, sampler u_smaaSearchSampler,
                       float2 e, float offset)
{
  // The texture is flipped vertically, with left and right cases taking half
  // of the space horizontally.
  float2 scale = SMAA_SEARCHTEX_SIZE * float2(0.5, -1.0);
  float2 bias = SMAA_SEARCHTEX_SIZE * float2(offset, 1.0);
  
  // Scale and bias to access texel centers.
  scale += float2(-1.0,  1.0);
  bias += float2( 0.5, -0.5);
  
  // Convert from pixel coordinates to texcoords.
  // (We use SMAA_SEARCHTEX_PACKED_SIZE because the texture is cropped).
  scale *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
  bias *= 1.0 / SMAA_SEARCHTEX_PACKED_SIZE;
  
  // Lookup the search texture.
  return u_smaaSearch.sample(u_smaaSearchSampler, scale * e + bias, level(0)).a;
}

float SMAASearchXLeft(texture2d<float> u_colorTex, sampler u_colorTexSampler,
                      texture2d<float> u_smaaSearch, sampler u_smaaSearchSampler,
                      float2 texcoord, float end, float4 framebufferMetrics)
{
  float2 e = float2(0.0, 1.0);
  while (texcoord.x > end && e.g > kActivationThreshold && e.r == 0.0)
  {
    e = u_colorTex.sample(u_colorTexSampler, texcoord, level(0)).rg;
    texcoord = float2(-2.0, 0.0) * framebufferMetrics.xy + texcoord;
  }
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(u_smaaSearch, u_smaaSearchSampler, e, 0.0);
  return framebufferMetrics.x * offset + texcoord.x;
}

float SMAASearchXRight(texture2d<float> u_colorTex, sampler u_colorTexSampler,
                       texture2d<float> u_smaaSearch, sampler u_smaaSearchSampler,
                       float2 texcoord, float end, float4 framebufferMetrics)
{
  float2 e = float2(0.0, 1.0);
  while (texcoord.x < end && e.g > kActivationThreshold && e.r == 0.0)
  {
    e = u_colorTex.sample(u_colorTexSampler, texcoord, level(0)).rg;
    texcoord = float2(2.0, 0.0) * framebufferMetrics.xy + texcoord;
  }
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(u_smaaSearch, u_smaaSearchSampler, e, 0.5);
  return -framebufferMetrics.x * offset + texcoord.x;
}

float SMAASearchYUp(texture2d<float> u_colorTex, sampler u_colorTexSampler,
                    texture2d<float> u_smaaSearch, sampler u_smaaSearchSampler,
                    float2 texcoord, float end, float4 framebufferMetrics)
{
  float2 e = float2(1.0, 0.0);
  while (texcoord.y > end && e.r > kActivationThreshold && e.g == 0.0)
  {
    e = u_colorTex.sample(u_colorTexSampler, texcoord, level(0)).rg;
    texcoord = float2(0.0, -2.0) * framebufferMetrics.xy + texcoord;
  }
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(u_smaaSearch, u_smaaSearchSampler, e.gr, 0.0);
  return framebufferMetrics.y * offset + texcoord.y;
}

float SMAASearchYDown(texture2d<float> u_colorTex, sampler u_colorTexSampler,
                      texture2d<float> u_smaaSearch, sampler u_smaaSearchSampler,
                      float2 texcoord, float end, float4 framebufferMetrics)
{
  float2 e = float2(1.0, 0.0);
  while (texcoord.y < end && e.r > kActivationThreshold && e.g == 0.0)
  {
    e = u_colorTex.sample(u_colorTexSampler, texcoord, level(0)).rg;
    texcoord = float2(0.0, 2.0) * framebufferMetrics.xy + texcoord;
  }
  float offset = 3.25 - (255.0 / 127.0) * SMAASearchLength(u_smaaSearch, u_smaaSearchSampler, e.gr, 0.5);
  return -framebufferMetrics.y * offset + texcoord.y;
}

// Here, we have the distance and both crossing edges. So, what are the areas
// at each side of current edge?
float2 SMAAArea(texture2d<float> u_smaaArea, sampler u_smaaAreaSampler, float2 dist, float e1, float e2)
{
  // Rounding prevents precision errors of bilinear filtering.
  float2 texcoord = kAreaTexMaxDistance * round(4.0 * float2(e1, e2)) + dist;
  // We do a scale and bias for mapping to texel space.
  texcoord = SMAA_AREATEX_PIXEL_SIZE * (texcoord + 0.5);
  return u_smaaArea.sample(u_smaaAreaSampler, texcoord, level(0)).rg;
}

fragment float4 fsSmaaBlendingWeight(const SmaaBlendingWeightFragment_T in [[stage_in]],
                                     constant Uniforms_T & uniforms [[buffer(0)]],
                                     texture2d<float> u_colorTex [[texture(0)]],
                                     sampler u_colorTexSampler [[sampler(0)]],
                                     texture2d<float> u_smaaArea [[texture(1)]],
                                     sampler u_smaaAreaSampler [[sampler(1)]],
                                     texture2d<float> u_smaaSearch [[texture(2)]],
                                     sampler u_smaaSearchSampler [[sampler(2)]])
{
  float4 weights = float4(0.0, 0.0, 0.0, 0.0);
  float2 e = u_colorTex.sample(u_colorTexSampler, in.coords.xy).rg;
  
  if (e.g > 0.0) // Edge at north
  {
    float2 d;
    
    // Find the distance to the left.
    float3 coords;
    coords.x = SMAASearchXLeft(u_colorTex, u_colorTexSampler, u_smaaSearch, u_smaaSearchSampler,
                               in.offset0.xy, in.offset2.x, uniforms.u_framebufferMetrics);
    coords.y = in.offset1.y;
    d.x = coords.x;
    
    // Now fetch the left crossing edges, two at a time using bilinear
    // filtering. Sampling at -0.25 enables to discern what value each edge has.
    float e1 = u_colorTex.sample(u_colorTexSampler, coords.xy, level(0)).r;
    
    // Find the distance to the right.
    coords.z = SMAASearchXRight(u_colorTex, u_colorTexSampler, u_smaaSearch, u_smaaSearchSampler,
                                in.offset0.zw, in.offset2.y, uniforms.u_framebufferMetrics);
    d.y = coords.z;
    
    // We want the distances to be in pixel units (doing this here allow to
    // better interleave arithmetic and memory accesses).
    d = abs(round(uniforms.u_framebufferMetrics.zz * d - in.coords.zz));
    
    // SMAAArea below needs a sqrt, as the areas texture is compressed
    // quadratically.
    float2 sqrtD = sqrt(d);
    
    // Fetch the right crossing edges.
    float e2 = u_colorTex.sample(u_colorTexSampler, coords.zy, level(0), int2(1, 0)).r;
    
    // Here we know how this pattern looks like, now it is time for getting
    // the actual area.
    weights.rg = SMAAArea(u_smaaArea, u_smaaAreaSampler, sqrtD, e1, e2);
  }
  
  if (e.r > 0.0) // Edge at west
  {
    float2 d;
    
    // Find the distance to the top.
    float3 coords;
    coords.y = SMAASearchYUp(u_colorTex, u_colorTexSampler, u_smaaSearch, u_smaaSearchSampler,
                             in.offset1.xy, in.offset2.z, uniforms.u_framebufferMetrics);
    coords.x = in.offset0.x;
    d.x = coords.y;
    
    // Fetch the top crossing edges.
    float e1 = u_colorTex.sample(u_colorTexSampler, coords.xy, level(0)).g;
    
    // Find the distance to the bottom.
    coords.z = SMAASearchYDown(u_colorTex, u_colorTexSampler, u_smaaSearch, u_smaaSearchSampler,
                               in.offset1.zw, in.offset2.w, uniforms.u_framebufferMetrics);
    d.y = coords.z;
    
    // We want the distances to be in pixel units.
    d = abs(round(uniforms.u_framebufferMetrics.ww * d - in.coords.ww));
    
    // SMAAArea below needs a sqrt, as the areas texture is compressed
    // quadratically.
    float2 sqrtD = sqrt(d);
    
    // Fetch the bottom crossing edges.
    float e2 = u_colorTex.sample(u_colorTexSampler, coords.xz, level(0), int2(0, 1)).g;
    
    // Get the area for this direction.
    weights.ba = SMAAArea(u_smaaArea, u_smaaAreaSampler, sqrtD, e1, e2);
  }
  
  return weights;
}

// SmaaFinal

typedef struct
{
  float4 position [[position]];
  float4 offset;
  float2 texCoords;
} SmaaFinalFragment_T;

vertex SmaaFinalFragment_T vsSmaaFinal(const SmaaVertex_T in [[stage_in]],
                                       constant Uniforms_T & uniforms [[buffer(1)]])
{
  SmaaFinalFragment_T out;
  float2 tcoord = float2(in.a_tcoord.x, 1.0 - in.a_tcoord.y);
  out.position = float4(in.a_pos, 0.0, 1.0);
  out.offset = uniforms.u_framebufferMetrics.xyxy * float4(1.0, 0.0, 0.0, 1.0) + tcoord.xyxy;
  out.texCoords = tcoord;
  
  return out;
}

fragment float4 fsSmaaFinal(const SmaaFinalFragment_T in [[stage_in]],
                            constant Uniforms_T & uniforms [[buffer(0)]],
                            texture2d<float> u_colorTex [[texture(0)]],
                            sampler u_colorTexSampler [[sampler(0)]],
                            texture2d<float> u_blendingWeightTex [[texture(1)]],
                            sampler u_blendingWeightTexSampler [[sampler(1)]])
{
  // Fetch the blending weights for current pixel.
  float4 a;
  a.x = u_blendingWeightTex.sample(u_blendingWeightTexSampler, in.offset.xy).a; // Right
  a.y = u_blendingWeightTex.sample(u_blendingWeightTexSampler, in.offset.zw).g; // Top
  a.wz = u_blendingWeightTex.sample(u_blendingWeightTexSampler, in.texCoords).xz; // Bottom / Left
  
  // Is there any blending weight with a value greater than 0.0?
  if (dot(a, float4(1.0, 1.0, 1.0, 1.0)) < 1e-5)
    return u_colorTex.sample(u_colorTexSampler, in.texCoords);

  // Calculate the blending offsets.
  float4 blendingOffset = float4(0.0, a.y, 0.0, a.w);
  float2 blendingWeight = a.yw;
  if (max(a.x, a.z) > max(a.y, a.w))
  {
    blendingOffset = float4(a.x, 0.0, a.z, 0.0);
    blendingWeight = a.xz;
  }
  blendingWeight /= dot(blendingWeight, float2(1.0, 1.0));
  
  // Calculate the texture coordinates.
  float4 bc = blendingOffset * float4(uniforms.u_framebufferMetrics.xy, -uniforms.u_framebufferMetrics.xy);
  bc += in.texCoords.xyxy;
  
  // We exploit bilinear filtering to mix current pixel with the chosen neighbor.
  float4 color = blendingWeight.x * u_colorTex.sample(u_colorTexSampler, bc.xy, level(0));
  color += blendingWeight.y * u_colorTex.sample(u_colorTexSampler, bc.zw, level(0));
  return color;
}
