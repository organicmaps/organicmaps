#ifdef GL_FRAGMENT_PRECISION_HIGH
  #define MAXPREC highp
#else
  #define MAXPREC mediump
#endif
varying MAXPREC float v_dx;
varying MAXPREC vec4 v_radius;
varying MAXPREC vec2 v_type;

varying lowp vec3 v_color;
varying lowp vec3 v_mask;

~getTexel~

void sphere_join(MAXPREC float gip2, lowp vec4 baseColor, lowp vec4 outlineColor)
{
  MAXPREC float r = v_radius.y;
  if (gip2 > v_radius.w * v_radius.w)
  {
    gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.y - sqrt(gip2)) / (v_radius.y - v_radius.w));
    if (v_type.x > 0.6)
      gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.z - abs(v_dx * v_radius.z)) / r * 2.0);
  }
  else
  {
    if (v_type.x > 0.6)
    {
      if (abs(v_dx*v_radius.z) / 2.0 >= v_radius.z / 2.0 - r + v_radius.w)
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.z - abs(v_dx * v_radius.z)) / (r-v_radius.w));
    }
  }
}

void main(void)
{
  lowp float alfa = getTexel(int(v_mask.z), v_mask.xy).a;
  lowp vec4 color = getTexel(int(v_color.z), v_color.xy) * alfa;
  lowp vec4 outlineColor = vec4(0.0, 0.0, 0.0, 0.0);
  MAXPREC float r = v_radius.y;
  MAXPREC float dist = abs(v_radius.x);
  gl_FragColor = color;
  if (v_type.x > 0.5)
  {
    MAXPREC float coord = (v_dx + 1.0) * v_radius.y / 2.0;
    if (v_type.y > 0.5)
    {
      if (coord > v_radius.w)
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (1.0 - (coord - v_radius.w) / (v_radius.y - v_radius.w)));

      if (dist > v_radius.w)
      {
        lowp float alpha = min((1.0 - (coord - v_radius.w) / (v_radius.y - v_radius.w)), (v_radius.y - dist) / (v_radius.y - v_radius.w));
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * alpha);
      }

    }
    else
    {
      if (coord < v_radius.y - v_radius.w)
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * coord / (v_radius.y - v_radius.w));

      if (dist > v_radius.w)
      {
        lowp float alpha = min(coord / (v_radius.y - v_radius.w), (v_radius.y - dist) / (v_radius.y - v_radius.w));
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * alpha);
      }
    }
  }
  else if (v_type.x < -0.5)
  {
    MAXPREC float y = (v_dx + 1.0) * v_radius.y / 2.0;
    if (v_type.y < 0.5)
      y = v_radius.y - (v_dx + 1.0) * v_radius.y / 2.0;

    MAXPREC float sq = dist*dist + y*y;
    if (sq >= v_radius.y * v_radius.y)
      discard;
    if (sq > v_radius.w * v_radius.w)
      gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.y - sqrt(sq)) / (v_radius.y - v_radius.w));
  }
  else
  {
    if (dist > v_radius.w)
      gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.y - dist) / (v_radius.y - v_radius.w));

    if (v_type.y>0.1)
    {
      if (v_dx >= 1.0)
      {
        MAXPREC float y = (v_dx - 1.0) * v_radius.z / 2.0;
        MAXPREC float gip2 = dist*dist + y*y;
        if(gip2 > v_radius.y * v_radius.y)
          discard;
        else
          sphere_join(gip2, color, outlineColor);
      }
      else if (v_dx <= -1.0)
      {
        MAXPREC float y = (v_dx + 1.0) * v_radius.z / 2.0;
        MAXPREC float gip2 = dist*dist + y*y;
        if(gip2 > v_radius.y * v_radius.y)
          discard;
        else
          sphere_join(gip2, color, outlineColor);
      }
    }
  }
}
