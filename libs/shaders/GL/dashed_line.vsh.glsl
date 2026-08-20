layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_colorTexCoord;
layout (location = 3) in vec4 a_maskTexCoord;

layout (location = 0) out vec2 v_colorTexCoord;
layout (location = 1) out vec2 v_maskTexCoord;  // x = unbounded offset along the line, y = mask V
layout (location = 2) out vec2 v_maskBase;      // x = mask rect min U, y = mask rect width U
//layout (location = 3) out vec2 v_halfLength;

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
  // The unbounded offset (distance in mask-length units) wraps in the fragment shader.
  float uOffset = length(vec4(kShapeCoordScalar, 0, 0, 0) * u_modelView) * a_maskTexCoord.x;
  v_colorTexCoord = a_colorTexCoord;
  v_maskTexCoord = vec2(uOffset, a_maskTexCoord.w);
  v_maskBase = a_maskTexCoord.yz;
  //v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z));
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
