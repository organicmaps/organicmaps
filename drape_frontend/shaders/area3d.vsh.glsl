attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;
uniform float zScale;

varying vec2 v_colorTexCoords;
varying float v_intensity;

const vec4 kNormalizedLightDir = vec4(0.3162, 0.0, 0.9486, 0.0);

void main()
{
  vec4 pos = vec4(a_position, 1.0) * modelView;
  
  vec4 normal = vec4(a_position + a_normal, 1.0) * modelView;
  normal.xyw = (normal * projection).xyw;
  normal.z = normal.z * zScale;

  pos.xyw = (pos * projection).xyw;
  pos.z = a_position.z * zScale;

  vec4 normDir = normal - pos;
  if (dot(normDir, normDir) != 0.0)
    v_intensity = max(0.0, -dot(kNormalizedLightDir, normalize(normDir)));
  else
    v_intensity = 0.0;

  gl_Position = pivotTransform * pos;
  v_colorTexCoords = a_colorTexCoords;
}
