attribute vec3 a_position;
attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;

void main(void)
{
  // Here we intentionally decrease precision of 'pivot' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pivot = vec4(a_position, 1) * modelView;
  vec4 offset = vec4(a_normal, 0, 0) * projection;

  vec4 projectedPivot = pivot * projection;
  vec4 transformedPivot = pivotTransform * projectedPivot;

  vec4 scale = pivotTransform * vec4(1.0, -1.0, 0, 1.0);
  gl_Position = transformedPivot + vec4(offset.xy * transformedPivot.w / scale.w * scale.x, 0, 0);

  v_colorTexCoords = a_colorTexCoords;
}
