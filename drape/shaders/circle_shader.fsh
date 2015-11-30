uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec2 v_colorTexCoords;
varying vec3 v_radius;

void main(void)
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  float stepValue = step(v_radius.z * v_radius.z, v_radius.x * v_radius.x + v_radius.y * v_radius.y);
  finalColor.a = finalColor.a * u_opacity * (1.0 - stepValue);
  gl_FragColor = finalColor;
}
