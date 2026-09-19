layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

layout (location = 0) out float v_intensity;

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
  float u_dummy1;
  float u_dummy2;
  vec4 u_terrainLightDir;
};

void main()
{
  vec4 pos = vec4(a_position, 1) * u_modelView * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  // The Lambert intensity relative to the flat ground under the CURRENT light (the
  // light z IS the flat intensity): the light moves per frame, the mesh normals do
  // not. The shadow half is gamma-lifted (u_terrainLightDir.w) so the gentle slopes
  // stay visible; see RuleDrawer::DrawTerrainShade for the mesh/normals side.
  float intensity = dot(normalize(a_normal), u_terrainLightDir.xyz);
  float flatIntensity = u_terrainLightDir.z;
  float rel = (intensity - flatIntensity) / (intensity < flatIntensity ? flatIntensity : 1.0 - flatIntensity);
  v_intensity = rel < 0.0 ? -pow(-rel, u_terrainLightDir.w) : rel;
}
