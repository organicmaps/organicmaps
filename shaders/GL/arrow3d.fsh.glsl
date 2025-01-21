in vec3 v_normal;

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

uniform vec4 u_color;

out vec4 v_FragColor;

void main()
{
  float phongDiffuse = max(0.0, -dot(lightDir, v_normal));
  v_FragColor = vec4((phongDiffuse * 0.5 + 0.5) * u_color.rgb, u_color.a);
}
