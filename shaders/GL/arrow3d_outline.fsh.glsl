layout (location = 0) in float v_intensity;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
  mat4 u_transform;
  mat4 u_normalTransform;
  vec4 u_color;
  vec2 u_texCoordFlipping;
};

void main()
{
  v_FragColor = vec4(u_color.rgb, u_color.a * smoothstep(0.7, 1.0, v_intensity));
}
