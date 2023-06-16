varying vec3 v_normal;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
#endif

const vec3 lightDir = vec3(0.316, 0.0, 0.948);

uniform vec4 u_color;

void main()
{
  float phongDiffuse = max(0.0, -dot(lightDir, v_normal));
  vec4 resColor = vec4((phongDiffuse * 0.5 + 0.5) * u_color.rgb, u_color.a);
  gl_FragColor = samsungGoogleNexusWorkaround(resColor);
}
