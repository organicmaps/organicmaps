layout (location = 0) in vec3 a_position;
layout (location = 1) in vec4 a_normal;
layout (location = 2) in vec4 a_color;

layout (location = 0) out vec3 v_radius;
layout (location = 1) out vec4 v_color;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_params;
  float u_lineHalfWidth;
  float u_maxRadius;
};

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    vec2 norm = normal * u_lineHalfWidth;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }
  transformedAxisPos += a_normal.zw * u_lineHalfWidth;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  v_color = a_color;
  v_radius = vec3(a_normal.zw, u_maxRadius) * u_lineHalfWidth;
}
