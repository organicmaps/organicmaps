#define TILE_BACKGROUND_MAX_COUNT 64

layout (location = 0) out vec3 v_texCoords;

// Note: std430 is important here, arrays must be 4-bytes aligned
layout (binding = 0, std430) readonly buffer UBO
{
  vec4 u_tileCoordsMinMax[TILE_BACKGROUND_MAX_COUNT];
  int u_textureIndex[TILE_BACKGROUND_MAX_COUNT];
  mat4 u_modelView;
  mat4 u_projection;
  mat4 u_pivotTransform;
};

void main()
{
  // Quad vertices: (0,0), (1,0), (0,1), (1,1) based on gl_VertexID
  vec2 quadVertex = vec2(gl_VertexIndex & 1, (gl_VertexIndex >> 1) & 1);
  
  vec4 tileCoordsMinMax = u_tileCoordsMinMax[gl_InstanceIndex];
  vec2 worldPos = mix(tileCoordsMinMax.xy, tileCoordsMinMax.zw, quadVertex);
  vec4 pos = vec4(worldPos, 0.0, 1.0) * u_modelView * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
  
  v_texCoords = vec3(quadVertex, float(u_textureIndex[gl_InstanceIndex]));
}
