varying vec3 v_length;

uniform sampler2D u_colorTex;
uniform vec4 u_textureRect;

uniform mat4 u_arrowBorders;

vec2 calculateUv(vec2 len, vec4 arrowBorder)
{
  vec2 uv = vec2(0, 0);
  if (len.x >= arrowBorder.x && len.x <= arrowBorder.z)
  {
    float coef = clamp((len.x - arrowBorder.x) / (arrowBorder.z - arrowBorder.x), 0.0, 1.0);
    float u = mix(arrowBorder.y, arrowBorder.w, coef);
    float v = 0.5 * len.y + 0.5;
    uv = vec2(mix(u_textureRect.x, u_textureRect.z, u), mix(u_textureRect.y, u_textureRect.w, v));
  }
  return uv;
}

void main(void)
{
  vec4 arrowBorder = u_arrowBorders[0];
  vec2 uv = calculateUv(v_length.xy, u_arrowBorders[0]) +
            calculateUv(v_length.xy, u_arrowBorders[1]) +
            calculateUv(v_length.xy, u_arrowBorders[2]) +
            calculateUv(v_length.xy, u_arrowBorders[3]);

  vec4 color = texture2D(u_colorTex, uv);
  gl_FragColor = color;
}
