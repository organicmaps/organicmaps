attribute highp vec4 position;
attribute highp vec4 normal;

uniform highp mat4 modelView;
uniform highp mat4 projection;

void main(void)
{
    gl_Position = (position + normal) * modelView * projection;
}
