uniform sampler2D u_textures[8];
varying highp vec4 v_texCoords;

const int Index0  = 0;
const int Index1  = 1;
const int Index2  = 2;
const int Index3  = 3;
const int Index4  = 4;
const int Index5  = 5;
const int Index6  = 6;
const int Index7  = 7;

void main(void)
{
  int index = int(floor(v_texCoords.z));
  highp vec4 color;
  if (index == Index0)
    color = texture2D(u_textures[Index0], v_texCoords.st);
  else if (index == Index1)
    color = texture2D(u_textures[Index1], v_texCoords.st);
  else if (index == Index2)
    color = texture2D(u_textures[Index2], v_texCoords.st);
  else if (index == Index3)
    color = texture2D(u_textures[Index3], v_texCoords.st);
  else if (index == Index4)
    color = texture2D(u_textures[Index4], v_texCoords.st);
  else if (index == Index5)
    color = texture2D(u_textures[Index5], v_texCoords.st);
  else if (index == Index6)
    color = texture2D(u_textures[Index6], v_texCoords.st);
  else if (index == Index7)
    color = texture2D(u_textures[Index7], v_texCoords.st);

  gl_FragColor = color;
}
