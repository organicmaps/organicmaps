attribute highp vec4 position;
attribute highp vec4 direction;

attribute highp vec4 a_vertType;

varying highp vec4  v_vertType;
varying highp vec4  v_distanceInfo;

uniform highp mat4 modelView;
uniform highp mat4 projection;

void main(void)
{
  highp float half_w = direction.z;
  highp float vertexType = a_vertType.x;

  highp vec4 pos;
  highp vec4 pivot = position * modelView;

  if (vertexType < 0.0)
  {
    highp vec4 n = vec4(-direction.y, direction.x, 0, 0);
    highp vec4 d = vec4( direction.x, direction.y, 0, 0);

    highp vec4 pn = normalize(n * modelView) * half_w;
    highp float quadWidth = a_vertType.y <= 0.0 ? 2.0 * abs(half_w) : abs(half_w);
    highp vec4 pd = normalize(d * modelView) * quadWidth;

    pos = (pn - pd + pivot);
  }
  else
  {
    highp vec4 n = vec4(-direction.y, direction.x, 0, 0);
    highp vec4 pn = normalize(n * modelView) * half_w;

    pos = (pn + pivot);
  }

  v_distanceInfo = vec4(pivot.x, pivot.y, pos.x, pos.y);

  v_vertType = a_vertType;
  v_vertType.w = 2.0 * abs(half_w);

  gl_Position = pos * projection;
}
