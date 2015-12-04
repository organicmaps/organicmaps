attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoords;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoords;
varying float v_intensity;

const vec4 lightDir = vec4(1.0, 0.0, 3.0, 0.0);

void main(void)
{
  vec4 pos = vec4(a_position, 1.0) * modelView;
  vec4 pos_dx = vec4(a_position.x + 1.0, a_position.y, 0.0, 1.0) * modelView;
  vec2 dx = pos_dx.xy / pos_dx.w - pos.xy / pos.w;
  
  float zScale = projection[0][0] * length(dx);
  
  vec4 normal = vec4(a_position + a_normal, 1.0) * modelView;
  normal.xyw = (normal * projection).xyw;
  normal.z = normal.z * zScale;

  pos.xyw = (pos * projection).xyw;
  pos.z = a_position.z * zScale;
  
  v_intensity = max(0.0, -dot(normalize(lightDir), normalize(normal - pos)));

  gl_Position = pivotTransform * pos;
  v_colorTexCoords = a_colorTexCoords;
}
