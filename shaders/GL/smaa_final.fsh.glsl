// Implementation of Subpixel Morphological Antialiasing (SMAA) is based on https://github.com/iryoku/smaa

uniform sampler2D u_colorTex;
uniform sampler2D u_blendingWeightTex;

uniform vec4 u_framebufferMetrics;

varying vec2 v_colorTexCoords;
varying vec4 v_offset;

#ifdef GLES3
  #define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0)
#else
  #define SMAASampleLevelZero(tex, coord) texture2D(tex, coord)
#endif

void main()
{
  // Fetch the blending weights for current pixel.
  vec4 a;
  a.x = texture2D(u_blendingWeightTex, v_offset.xy).a; // Right
  a.y = texture2D(u_blendingWeightTex, v_offset.zw).g; // Top
  a.wz = texture2D(u_blendingWeightTex, v_colorTexCoords).xz; // Bottom / Left

  // Is there any blending weight with a value greater than 0.0?
  if (dot(a, vec4(1.0, 1.0, 1.0, 1.0)) < 1e-5)
  {
    gl_FragColor = texture2D(u_colorTex, v_colorTexCoords);
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
    gl_FragColor = color;
  }
}
