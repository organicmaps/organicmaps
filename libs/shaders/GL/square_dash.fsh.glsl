layout (location = 0) in vec2 v_colorTexCoord;
layout (location = 1) in vec4 v_dash;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_isOutlinePass;
};

layout (binding = 1) uniform sampler2D u_colorTex;

void main()
{
  float start = min(floor(v_dash.x), v_dash.w);
  if (start < v_dash.z || v_dash.x - start > v_dash.y)
    discard;
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  color.a *= u_opacity;
  v_FragColor = color;
}
