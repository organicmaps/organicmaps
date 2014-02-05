uniform lowp vec4 u_color;

varying highp vec4  v_vertType;
varying highp vec4  v_distanceInfo;

highp float cap(lowp float type, highp float dx, highp float dy, highp float width)
{
  highp float hw = width/2.0;

  if (type == 0.0)
    return -(dx*dx + dy*dy) + hw*hw;
  else
    return 1.0;
}

highp float join(lowp float type, highp float dx, highp float dy, highp float width)
{
  return 1.0;
}

void main(void)
{
  lowp float vertType = v_vertType.x;

  if (vertType == 0.0)
  {
    gl_FragColor = u_color;
    return;
  }


  highp vec2  d     = v_distanceInfo.zw - v_distanceInfo.xy;
  highp float width = v_vertType.w;

  if (vertType > 0.0)
  {
    lowp float joinType = v_vertType.z;

    if ( join(joinType, d.x, d.y, width) > 0.0 )
    {
      gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
      return;
    }
    discard;
  }
  else if (vertType < 0.0)
  {
    lowp float capType = v_vertType.y;

    if ( cap(capType, d.x, d.y, width) > 0.0 )
    {
      gl_FragColor = u_color;
      return;
    }
    discard;
  }

  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
