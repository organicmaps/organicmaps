attribute vec3 a_pos;
attribute vec3 a_normal;

uniform mat4 m_transform;

varying float v_intensity;

const vec4 lightDir = vec4(1.0, 0.0, 1.0, 0.0);

void main()
{
  vec4 position = m_transform * vec4(a_pos, 1.0);
  vec4 normal = m_transform * vec4(a_normal + a_pos, 1.0);
  v_intensity = max(0.0, -dot(normalize(lightDir), normalize(normal - position)));
  gl_Position = position;
}

