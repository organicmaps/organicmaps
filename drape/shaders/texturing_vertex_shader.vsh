attribute mediump vec2 position;
attribute mediump float depth;
attribute mediump vec4 texCoords;

uniform highp mat4 modelViewProjectionMatrix;

varying highp vec4 varTexCoords;

void main(void)
{
    gl_Position = modelViewProjectionMatrix * vec4(position, depth, 1.0);
    varTexCoords = texCoords;
}
