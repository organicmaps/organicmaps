varying mediump vec3 v_color_index;

~getTexel~

void main(void)
{
  gl_FragColor = getTexel(int(v_color_index.z), v_color_index.xy);
}
