attribute vec3 a_position;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float zScale;

#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main()
{
  vec4 pos = vec4(a_position, 1.0) * modelView;
  pos.xyw = (pos * projection).xyw;
  pos.z = a_position.z * zScale;
  gl_Position = pivotTransform * pos;

#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
}
