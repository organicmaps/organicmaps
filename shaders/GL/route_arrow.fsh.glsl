// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented without discarding fragments from depth buffer.

layout (location = 0) in vec2 v_colorTexCoords;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_routeParams;
  vec4 u_color;
  vec4 u_maskColor;
  vec4 u_outlineColor;
  vec4 u_fakeColor;
  vec4 u_fakeOutlineColor;
  vec2 u_fakeBorders;
  vec2 u_pattern;
  vec2 u_angleCosSin;
  float u_arrowHalfWidth;
  float u_opacity;
};

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  if (finalColor.a < 0.01)
    discard;
  finalColor = vec4(mix(finalColor.rgb, u_maskColor.rgb, u_maskColor.a), finalColor.a);
  v_FragColor = finalColor;
}
