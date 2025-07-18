uniform sampler2D u_colorTex;
uniform float u_opacity;

varying vec2 v_colorTexCoord;

void main()
{
  vec4 color = texture2D(u_colorTex, v_colorTexCoord);
  gl_FragColor = vec4(color.rgb, u_opacity);
}
