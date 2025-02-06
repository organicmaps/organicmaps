#ifdef ENABLE_VTF
layout (location = 0) in LOW_P vec4 v_color;
#else
layout (location = 1) in vec2 v_colorTexCoord;
layout (binding = 1) uniform sampler2D u_colorTex;
#endif
layout (location = 2) in float v_lengthY;

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

const float kAntialiasingThreshold = 0.92;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_lengthY)));
  v_FragColor = color;
}
