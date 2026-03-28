layout (location = 0) in float v_side;
layout (location = 1) in vec4 v_colorCoords01;
layout (location = 2) in vec4 v_colorCoords23;
layout (location = 3) in float v_stripeCount;
layout (location = 4) in vec2 v_maskTexCoord;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

layout (binding = 1) uniform sampler2D u_colorTex;
layout (binding = 2) uniform sampler2D u_maskTex;

void main()
{
  // v_side interpolates from -1.0 (right) to +1.0 (left) across the line quad.
  float t = (v_side + 1.0) * 0.5;
  int idx = clamp(int(t * v_stripeCount), 0, int(v_stripeCount) - 1);

  vec2 coords[4] = vec2[4](v_colorCoords01.xy, v_colorCoords01.zw,
                            v_colorCoords23.xy, v_colorCoords23.zw);
  vec4 color = texture(u_colorTex, coords[idx]);
  float mask = texture(u_maskTex, v_maskTexCoord).r;
  color.a = color.a * mask * u_opacity;
  v_FragColor = color;
}
