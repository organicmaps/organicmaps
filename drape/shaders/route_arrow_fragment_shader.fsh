varying vec2 v_length;

uniform sampler2D u_colorTex;
uniform vec4 u_textureRect;

uniform mat4 u_arrowBorders;

void main(void)
{
  bool needDiscard = true;
  vec2 uv = vec2(0, 0);
  for (int i = 0; i < 4; i++)
  {
    vec4 arrowBorder = u_arrowBorders[i];
    if (v_length.x >= arrowBorder.x && v_length.x <= arrowBorder.z)
    {
      needDiscard = false;
      float coef = clamp((v_length.x - arrowBorder.x) / (arrowBorder.z - arrowBorder.x), 0.0, 1.0);
      float u = mix(arrowBorder.y, arrowBorder.w, coef);
      float v = 0.5 * v_length.y + 0.5;
      uv = vec2(mix(u_textureRect.x, u_textureRect.z, u), mix(u_textureRect.y, u_textureRect.w, v));
    }
  }

  if (needDiscard)
    discard;

  gl_FragColor = texture2D(u_colorTex, uv);
}
