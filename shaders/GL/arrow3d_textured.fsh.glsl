in vec3 v_normal;
in vec2 v_texCoords;

uniform sampler2D u_colorTex;

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

uniform vec4 u_color;

out vec4 v_FragColor;

void main()
{
  float phongDiffuse = max(0.0, -dot(lightDir, v_normal));
  vec4 color = texture(u_colorTex, v_texCoords) * u_color;
  v_FragColor = vec4((phongDiffuse * 0.5 + 0.5) * color.rgb, color.a);
}
