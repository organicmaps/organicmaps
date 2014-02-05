uniform lowp vec4 u_color;
uniform highp float u_width;

varying highp vec4  v_vertType;
varying highp vec4  v_distanceInfo;

highp float cap(lowp float type, highp float dx, highp float dy)
{
  if (type == 0.0)
  {
    highp float hw = u_width/2.0;
    return -(dx*dx + dy*dy) + hw*hw;
  }
  else
    return 1.0;
}

highp float join(lowp float type, highp float dx, highp float dy)
{
  if (type > 0.0)
  {
    highp float hw = u_width/2.0;
    return -(dx*dx + dy*dy) +  hw*hw;
  }
  else
    return 1.0;
}

void main(void)
{
  lowp float vertType = v_vertType.x;

  highp vec2  d     = v_distanceInfo.zw - v_distanceInfo.xy;
  if (vertType > 0.0)
  {
    lowp float joinType = v_vertType.y;
    if ( join(joinType, d.x, d.y) < 0.0 )
      discard;
  }
  else if (vertType < 0.0)
  {
    lowp float capType = v_vertType.y;
    if ( cap(capType, d.x, d.y) < 0.0 )
      discard;
  }

  gl_FragColor = u_color;
}
