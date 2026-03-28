layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec4 a_colorCoords01;
layout (location = 3) in vec4 a_colorCoords23;
layout (location = 4) in vec2 a_stripeInfo;

layout (location = 0) out float v_side;
layout (location = 1) out vec4 v_colorCoords01;
layout (location = 2) out vec4 v_colorCoords23;
layout (location = 3) out float v_stripeCount;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec2 u_contrastGamma;
  float u_opacity;
  float u_zScale;
  float u_interpolation;
  float u_isOutlinePass;
};

void main()
{
  vec2 normal = a_normal.xy;
  float halfWidth = length(normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (halfWidth != 0.0)
  {
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal,
                                                    u_modelView, halfWidth);
  }

  v_side = sign(a_normal.z);
  v_colorCoords01 = a_colorCoords01;
  v_colorCoords23 = a_colorCoords23;
  v_stripeCount = a_stripeInfo.x;

  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
