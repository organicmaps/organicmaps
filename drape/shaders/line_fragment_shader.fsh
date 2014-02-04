uniform lowp vec4 color;

varying highp vec4  v_vertType;
varying highp vec4  v_distanceInfo;

highp float cap(highp float type, highp float dx, highp float dy, highp float width)
{
  highp float hw = width/2.0;

  if (type == 0.0)
    return -(dx*dx + dy*dy) + hw*hw;
  else
    return 1.0;
}

highp float join(int type, highp float dx, highp float dy, highp float width)
{
  return 1.0;
}

void main(void)
{
  highp float vertType = v_vertType.x;

  if (vertType == 0.0)
  {
    gl_FragColor = color;
    return;
  }
  else if (vertType > 0.0)
  {
    highp float joinType = v_vertType.z;
    highp float dx = v_distanceInfo.z - v_distanceInfo.x;
    highp float dy = v_distanceInfo.w - v_distanceInfo.y;
    highp float width = v_vertType.w;

    if (join(int(joinType), dx, dy, width) > 0.0)
    {
      gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
      return;
    }
    else
    {
      discard;
    }
  }
  else if (vertType < 0.0)
  {
    highp float capType = v_vertType.y;
    highp float dx = v_distanceInfo.z - v_distanceInfo.x;
    highp float dy = v_distanceInfo.w - v_distanceInfo.y;
    highp float width = v_vertType.w;

    if ( cap(capType, dx, dy, width) > 0.0 )
    {
      gl_FragColor = color;
      return;
    }
    else
    {
      discard;
    }
  }

  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
