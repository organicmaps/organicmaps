#ifdef GL_FRAGMENT_PRECISION_HIGH
  #define MAXPREC highp
#else
  #define MAXPREC mediump
#endif
varying MAXPREC float v_dx;
varying MAXPREC vec4 v_radius;
varying MAXPREC vec4 v_centres;
varying MAXPREC vec2 v_type;

varying lowp vec4 v_colors;
varying lowp vec2 v_opacity;
varying mediump vec2 v_index;

~getTexel~

void sphere_join(MAXPREC float gip2, lowp vec4 baseColor, lowp vec4 outlineColor)
{
  MAXPREC float r = v_radius.y;
  gl_FragColor = baseColor;
  if (gip2 > v_radius.w * v_radius.w)
  {
    gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.y - sqrt(gip2)) / (v_radius.y - v_radius.w));
    if (v_type.x > 0.6)
      gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.z - abs(v_dx * v_radius.z)) / r * 2.0);
  }
  else
  {
    gl_FragColor = baseColor;
    if (v_type.x > 0.6)
    {
      if (abs(v_dx*v_radius.z) / 2.0 >= v_radius.z / 2.0 - r + v_radius.w)
        gl_FragColor = vec4(outlineColor.rgb, outlineColor.a * (v_radius.z - abs(v_dx * v_radius.z)) / (r-v_radius.w));
      else
        gl_FragColor = baseColor;
    }
  }
}

void main(void)
{
  int textureIndex = int(v_index.x);
  lowp float a = getTexel(int(v_index.y), v_opacity.xy).a;
  lowp vec4 baseColor = getTexel(textureIndex, v_colors.xy) * a;
  lowp vec4 outlineColor = getTexel(textureIndex, v_colors.zw) * a;
  MAXPREC float r = v_radius.y;
  MAXPREC float dist = abs(v_radius.x);
  gl_FragColor = baseColor;
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
    gl_FragColor = baseColor;
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
          sphere_join(gip2, baseColor, outlineColor);
      }
      else if (v_dx <= -1.0)
      {
        MAXPREC float y = (v_dx + 1.0) * v_radius.z / 2.0;
        MAXPREC float gip2 = dist*dist + y*y;
        if(gip2 > v_radius.y * v_radius.y)
          discard;
        else
          sphere_join(gip2, baseColor, outlineColor);
      }
    }
  }
}
