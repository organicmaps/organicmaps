layout (location = 0) in vec2 v_colorTexCoord;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_trafficParams;
  vec4 u_outlineColor;
  vec4 u_lightArrowColor;
  vec4 u_darkArrowColor;
  float u_outline;
  float u_opacity;
};

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  v_FragColor = vec4(color.rgb, u_opacity);
}
