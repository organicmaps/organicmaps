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

void main(void)
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * modelView;
  vec4 offset = vec4(a_normal.xy + a_colorTexCoords.zw, 0.0, 0.0) * projection;

  vec4 projectedPivot = pivot * projection;
  float logicZ = projectedPivot.z / projectedPivot.w;
  vec4 transformedPivot = pivotTransform * vec4(projectedPivot.xy, 0.0, projectedPivot.w);

  vec4 scale = pivotTransform * vec4(1.0, -1.0, 0.0, 1.0);
  gl_Position = vec4(transformedPivot.xy / transformedPivot.w, logicZ, 1.0) + vec4(offset.xy / scale.w * scale.x, 0.0, 0.0);

#ifdef ENABLE_VTF
  v_color = texture2D(u_colorTex, a_colorTexCoords.xy);
#else
  v_colorTexCoords = a_colorTexCoords.xy;
#endif
  v_normal = a_normal;
}
