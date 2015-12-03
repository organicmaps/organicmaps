varying vec3 v_length;

uniform sampler2D u_colorTex;
uniform vec4 u_textureRect;

uniform mat4 u_arrowBorders;

vec4 calculateColor(float t, float v, vec4 arrowBorder)
{
  float u = mix(arrowBorder.y, arrowBorder.w, t / arrowBorder.z);
  vec2 uv = vec2(mix(u_textureRect.x, u_textureRect.z, u), v);
  return texture2D(u_colorTex, uv);
}

void main(void)
{
  // DO NOT optimize conditions/add loops! After doing changes in this shader, please, test
  // it on Samsung Galaxy S5 mini because the device has a lot of problems with compiling
  // and executing shaders which work great on other devices.
  float v = mix(u_textureRect.y, u_textureRect.w, clamp(0.5 * v_length.y + 0.5, 0.0, 1.0));

  vec4 finalColor = vec4(0, 0, 0, 0);
  float t = v_length.x - u_arrowBorders[0].x;
  if (t >= 0.0 && t <= u_arrowBorders[0].z)
    finalColor = calculateColor(t, v, u_arrowBorders[0]);

  t = v_length.x - u_arrowBorders[1].x;
  if (t >= 0.0 && t <= u_arrowBorders[1].z)
    finalColor = calculateColor(t, v, u_arrowBorders[1]);

  t = v_length.x - u_arrowBorders[2].x;
  if (t >= 0.0 && t <= u_arrowBorders[2].z)
    finalColor = calculateColor(t, v, u_arrowBorders[2]);

  t = v_length.x - u_arrowBorders[3].x;
  if (t >= 0.0 && t <= u_arrowBorders[3].z)
    finalColor = calculateColor(t, v, u_arrowBorders[3]);

  gl_FragColor = finalColor;
}
