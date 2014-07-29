varying highp float v_dx;
varying highp vec4 v_radius;
varying highp vec4 v_centres;
varying highp vec2 v_type;

varying lowp vec4 baseColor;
varying lowp vec4 outlineColor;

void sphere_join(highp float gip2)
{
  highp float r = v_radius.y;
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
  highp float r = v_radius.y;
  highp float dist = abs(v_radius.x);
  gl_FragColor = baseColor;
  if (v_type.x > 0.5)
  {
    highp float coord = (v_dx + 1.0) * v_radius.y / 2.0;
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
    highp float y = (v_dx + 1.0) * v_radius.y / 2.0;
    if (v_type.y < 0.5)
      y = v_radius.y - (v_dx + 1.0) * v_radius.y / 2.0;

    highp float sq = dist*dist + y*y;
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
        highp float y = (v_dx - 1.0) * v_radius.z / 2.0;
        highp float gip2 = dist*dist + y*y;
        if(gip2 > v_radius.y * v_radius.y)
          discard;
        else
          sphere_join(gip2);
      }
      else if (v_dx <= -1.0)
      {
        highp float y = (v_dx + 1.0) * v_radius.z / 2.0;
        highp float gip2 = dist*dist + y*y;
        if(gip2 > v_radius.y * v_radius.y)
          discard;
        else
          sphere_join(gip2);
      }
    }
  }
}
