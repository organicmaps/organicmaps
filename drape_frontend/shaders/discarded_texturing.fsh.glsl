// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec2 v_colorTexCoords;

void main()
{
  vec4 finalColor = texture2D(u_colorTex, v_colorTexCoords);
  finalColor.a *= u_opacity;
  if (finalColor.a < 0.01)
    discard;
  gl_FragColor = finalColor;
}
