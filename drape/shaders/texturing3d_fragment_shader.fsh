uniform sampler2D tex;
varying vec2 v_tcoord;

void main()
{
  gl_FragColor = texture2D(tex, v_tcoord);
}
