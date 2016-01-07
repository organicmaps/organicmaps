attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec3 v_radius;
#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main(void)
{
  lowp vec4 p = vec4(a_position, 1) * modelView;
  highp vec4 pos = vec4(a_normal.xy, 0, 0) + p;
  pos = pos * projection;

  float w = pos.w;
  pos.xyw = (pivotTransform * vec4(pos.xy, 0.0, w)).xyw;
  pos.z *= pos.w / w;
  gl_Position = pos;
#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoords = a_colorTexCoords;
#endif
  v_radius = a_normal;
}
