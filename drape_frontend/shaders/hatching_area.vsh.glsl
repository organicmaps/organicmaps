attribute vec3 a_position;
attribute vec2 a_colorTexCoords;
attribute vec2 a_maskTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif
varying vec2 v_maskTexCoords;

void main()
{
  vec4 pos = vec4(a_position, 1) * modelView * projection;
  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
  v_maskTexCoords = a_maskTexCoords;
}
