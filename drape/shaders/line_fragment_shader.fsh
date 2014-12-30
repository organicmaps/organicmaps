varying float v_dx;
varying vec4 v_radius;
varying vec2 v_type;

varying vec2 v_color;
varying vec2 v_mask;

uniform sampler2D u_colorTex;
uniform sampler2D u_maskTex;

void main(void)
{
  float squareDist = v_radius.x * v_radius.x;
  float squareRadius = v_radius.y * v_radius.y;

  float capY = (v_dx + 1.0 * sign(v_dx)) * v_radius.z / 2.0;
  float joinY = (v_dx + 1.0) * v_radius.y / 2.0;

  if (v_type.x < -0.5)
  {
    float y = joinY;
    if (v_type.y < 0.5)
      y = v_radius.y - joinY;

    if (squareDist + y * y > squareRadius)
      discard;
  }
  else if (v_type.y > 0.1 && abs(v_dx) >= 1.0 && (squareDist + capY > squareRadius))
      discard;

  vec4 color = texture2D(u_colorTex, v_color);
  color.a = texture2D(u_maskTex, v_mask).a;
  gl_FragColor = color;
}
