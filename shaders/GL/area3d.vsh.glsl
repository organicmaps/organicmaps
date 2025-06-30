in vec3 a_position;
in vec3 a_normal;
in vec2 a_colorTexCoords;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;
uniform float u_zScale;

out vec2 v_colorTexCoords;
out float v_intensity;

const vec4 kNormalizedLightDir = vec4(0.3162, 0.0, 0.9486, 0.0);

void main()
{
  vec4 pos = vec4(a_position, 1.0) * u_modelView;

  vec4 normal = vec4(a_position + a_normal, 1.0) * u_modelView;
  normal.xyw = (normal * u_projection).xyw;
  normal.z = normal.z * u_zScale;

  pos.xyw = (pos * u_projection).xyw;
  pos.z = a_position.z * u_zScale;

  vec4 normDir = normal - pos;
  if (dot(normDir, normDir) != 0.0)
    v_intensity = max(0.0, -dot(kNormalizedLightDir, normalize(normDir)));
  else
    v_intensity = 0.0;

  gl_Position = u_pivotTransform * pos;
#ifdef VULKAN
  gl_Position.y = -gl_Position.y;
  gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
#endif
  v_colorTexCoords = a_colorTexCoords;
}
