layout (location = 0) in vec2 v_colorTexCoords;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_position;
  vec2 u_lineParams;
  float u_accuracy;
  float u_zScale;
  float u_opacity;
  float u_azimut;
};

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  vec4 finalColor = texture(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  v_FragColor = finalColor;
}
