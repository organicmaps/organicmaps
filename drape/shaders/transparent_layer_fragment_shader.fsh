uniform sampler2D tex;
varying vec2 v_tcoord;

const float opacity = 0.7;

void main()
{
  gl_FragColor = texture2D(tex, v_tcoord);
  gl_FragColor.a = gl_FragColor.a * opacity;
}
