attribute highp vec4 position;
attribute highp vec4 direction;

uniform highp mat4 modelView;
uniform highp mat4 projection;

void main(void)
{
  float width = direction.z;
  vec4 n = vec4(-direction.y, direction.x, 0, 0);
  vec4 pn = normalize(n * modelView) * width;
  gl_Position = (pn + position * modelView) * projection;
}
