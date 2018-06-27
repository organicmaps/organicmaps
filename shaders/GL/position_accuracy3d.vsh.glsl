attribute vec2 a_normal;
attribute vec2 a_colorTexCoords;

uniform vec3 u_position;
uniform float u_accuracy;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float zScale;

varying vec2 v_colorTexCoords;

void main()
{
  vec4 position = vec4(u_position.xy, 0.0, 1.0) * modelView;
  vec4 normal = vec4(0.0, 0.0, 0.0, 0.0);
  if (dot(a_normal, a_normal) != 0.0)
    normal = vec4(normalize(a_normal) * u_accuracy, 0.0, 0.0);
  position = (position + normal) * projection;
  gl_Position = applyPivotTransform(position, pivotTransform, u_position.z * zScale);

  v_colorTexCoords = a_colorTexCoords;
}
