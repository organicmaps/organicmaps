uniform sampler2D textureUnit;
varying highp vec4 varTexCoords;

void main(void)
{
    gl_FragColor = texture2D(textureUnit, varTexCoords.st);
}
