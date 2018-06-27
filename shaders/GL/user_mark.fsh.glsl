// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec4 v_texCoords;
varying vec3 v_maskColor;

void main()
{
  vec4 color = texture2D(u_colorTex, v_texCoords.xy);
  vec4 bgColor = texture2D(u_colorTex, v_texCoords.zw) * vec4(v_maskColor, 1.0);
  vec4 finalColor = mix(color, mix(bgColor, color, color.a), bgColor.a);
  finalColor.a = clamp(color.a + bgColor.a, 0.0, 1.0) * u_opacity;
  if (finalColor.a < 0.01)
    discard;
  gl_FragColor = finalColor;
}
