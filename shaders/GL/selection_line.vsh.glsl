layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_normal;
layout (location = 2) in vec2 a_colorTexCoords;
layout (location = 3) in vec3 a_length;

#ifdef ENABLE_VTF
layout (location = 0) out LOW_P vec4 v_color;
#else
layout (location = 1) out vec2 v_colorTexCoord;
#endif
layout (location = 2) out float v_lengthY;

layout (binding = 0) uniform UBO
{
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
  vec4 u_position;
  vec2 u_lineParams;
  float u_accuracy;
  float u_zScale;
  float u_opacity;
  float u_azimut;
};

#ifdef ENABLE_VTF
layout (binding = 1) uniform sampler2D u_colorTex;
#endif

const float kAntialiasingThreshold = 0.92;

void main()
{
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  vec2 len = vec2(a_length.x, a_length.z);
  if (dot(a_normal, a_normal) != 0.0)
  {
    vec2 norm = a_normal * u_lineParams.x;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
    if (u_lineParams.y != 0.0)
      len = vec2(a_length.x + a_length.y * u_lineParams.y, a_length.z);
  }
#ifdef ENABLE_VTF
  v_color = texture(u_colorTex, a_colorTexCoords);
#else
  v_colorTexCoord = a_colorTexCoords;
#endif
  v_lengthY = len.y;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
