attribute vec4 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;
attribute vec2 a_maskTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float zScale;

varying vec2 v_colorTexCoords;
varying vec2 v_maskTexCoords;

void main()
{
  vec4 pivot = vec4(a_position.xyz, 1.0) * modelView;
  vec4 offset = vec4(a_normal, 0.0, 0.0) * projection;
  gl_Position = applyBillboardPivotTransform(pivot * projection, pivotTransform, a_position.w * zScale, offset.xy);

  v_colorTexCoords = a_colorTexCoords;
  v_maskTexCoords = a_maskTexCoords;
}
