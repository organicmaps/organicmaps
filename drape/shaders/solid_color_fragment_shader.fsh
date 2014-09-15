varying mediump vec3 v_color_index;

~getTexel~

void main(void)
{
  int textureIndex = int(v_color_index.z);
  gl_FragColor = getTexel(textureIndex, v_color_index.xy);
}
