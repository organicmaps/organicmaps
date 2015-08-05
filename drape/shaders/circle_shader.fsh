uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec2 v_colorTexCoords;
varying vec3 v_radius;

void main(void)
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  float stepValue = step(pow(v_radius.z, 2.0), pow(v_radius.x, 2.0) + pow(v_radius.y, 2.0));
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  gl_FragColor = finalColor;
}
