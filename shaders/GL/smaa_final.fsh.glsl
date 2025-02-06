// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa
layout (location = 0) in vec2 v_colorTexCoords;
layout (location = 1) in vec4 v_offset;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  vec4 u_framebufferMetrics;
};

layout (binding = 1) uniform sampler2D u_colorTex;

layout (binding = 2) uniform sampler2D u_blendingWeightTex;

#define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0)

void main()
{
  // Fetch the blending weights for current pixel.
  vec4 a;
  a.x = texture(u_blendingWeightTex, v_offset.xy).a; // Right
  a.y = texture(u_blendingWeightTex, v_offset.zw).g; // Top
  a.wz = texture(u_blendingWeightTex, v_colorTexCoords).xz; // Bottom / Left
  // Is there any blending weight with a value greater than 0.0?
  if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5)
  {
    v_FragColor = texture(u_colorTex, v_colorTexCoords);
  }
  else
  {
    // Calculate the blending offsets.
    vec4 blendingOffset = vec4(0.0, a.y, 0.0, a.w);
    vec2 blendingWeight = a.yw;
    if (max(a.x, a.z) > max(a.y, a.w))
    {
      blendingOffset = vec4(a.x, 0.0, a.z, 0.0);
      blendingWeight = a.xz;
    }
    blendingWeight /= dot(blendingWeight, vec2(1.0, 1.0));
    // Calculate the texture coordinates.
    vec4 bc = blendingOffset * vec4(u_framebufferMetrics.xy, -u_framebufferMetrics.xy);
    bc += v_colorTexCoords.xyxy;
    // We exploit bilinear filtering to mix current pixel with the chosen neighbor.
    vec4 color = blendingWeight.x * SMAASampleLevelZero(u_colorTex, bc.xy);
    color += blendingWeight.y * SMAASampleLevelZero(u_colorTex, bc.zw);
    v_FragColor = color;
  }
}
