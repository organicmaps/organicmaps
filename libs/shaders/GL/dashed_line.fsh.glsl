layout (location = 0) in vec2 v_colorTexCoord;
layout (location = 1) in vec2 v_maskTexCoord;
//layout (location = 2) in vec2 v_halfLength;

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

//const float aaPixelsCount = 2.5;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  float mask = texture(u_maskTex, v_maskTexCoord).r;
  color.a = color.a * mask * u_opacity;
  // Disabled too agressive AA-like blurring of edges,
  // see https://github.com/organicmaps/organicmaps/issues/6583.
  //float currentW = abs(v_halfLength.x);
  //float diff = v_halfLength.y - currentW;
  //color.a *= mix(0.3, 1.0, clamp(diff / aaPixelsCount, 0.0, 1.0));
  v_FragColor = color;
}
