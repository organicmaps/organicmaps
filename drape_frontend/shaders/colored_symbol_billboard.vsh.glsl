attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec4 v_normal;
#ifdef ENABLE_VTF
uniform sampler2D u_colorTex;
varying lowp vec4 v_color;
#else
varying vec2 v_colorTexCoords;
#endif

void main()
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * modelView;
  vec4 offset = vec4(a_normal.xy + a_colorTexCoords.zw, 0.0, 0.0) * projection;
  gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, 0.0, offset.xy);

#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords.xy);
#else
  v_colorTexCoords = a_colorTexCoords.xy;
#endif
  v_normal = a_normal;
}
