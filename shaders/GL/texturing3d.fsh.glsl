uniform sampler2D u_colorTex;
uniform float u_opacity;

in vec2 v_colorTexCoords;
in float v_intensity;

out vec4 v_FragColor;

void main()
{
  vec4 finalColor = vec4(texture(u_colorTex, v_colorTexCoords).rgb, u_opacity);
  v_FragColor = vec4((v_intensity * 0.2 + 0.8) * finalColor.rgb, finalColor.a);
}
