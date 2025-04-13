attribute vec3 a_position;
attribute vec4 a_normal;
attribute vec4 a_colorTexCoord;

uniform mat4 u_modelView;
uniform mat4 u_projection;
uniform mat4 u_pivotTransform;

uniform vec4 u_trafficParams;

varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying float v_halfLength;

const float kArrowVSize = 0.25;

void main()
{
  vec2 normal = a_normal.xy;
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * u_modelView).xy;
  if (dot(normal, normal) != 0.0)
  {
    vec2 norm = normal * u_trafficParams.x;
    if (a_normal.z < 0.0)
      norm = normal * u_trafficParams.y;
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + norm,
                                                    u_modelView, length(norm));
  }

  float uOffset = length(vec4(kShapeCoordScalar, 0, 0, 0) * u_modelView) * a_normal.w;
  v_colorTexCoord = a_colorTexCoord.xy;
  float v = mix(a_colorTexCoord.z, a_colorTexCoord.z + kArrowVSize, 0.5 * a_normal.z + 0.5);
  v_maskTexCoord = vec2(uOffset * u_trafficParams.z, v) * u_trafficParams.w;
  v_maskTexCoord.x *= step(a_colorTexCoord.w, v_maskTexCoord.x);
  v_halfLength = a_normal.z;
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * u_projection;
  gl_Position = applyPivotTransform(pos, u_pivotTransform, 0.0);
}
