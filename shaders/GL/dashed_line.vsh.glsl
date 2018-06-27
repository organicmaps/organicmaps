attribute vec3 a_position;
attribute vec3 a_normal;
attribute vec2 a_colorTexCoord;
attribute vec4 a_maskTexCoord;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

varying vec2 v_colorTexCoord;
varying vec2 v_maskTexCoord;
varying vec2 v_halfLength;

void main()
{
  vec2 normal = a_normal.xy;
  float halfWidth = length(normal);
  vec2 transformedAxisPos = (vec4(a_position.xy, 0.0, 1.0) * modelView).xy;
  if (halfWidth != 0.0)
  {
    transformedAxisPos = calcLineTransformedAxisPos(transformedAxisPos, a_position.xy + normal,
                                                    modelView, halfWidth);
  }

  float uOffset = min(length(vec4(kShapeCoordScalar, 0, 0, 0) * modelView) * a_maskTexCoord.x, 1.0);
  v_colorTexCoord = a_colorTexCoord;
  v_maskTexCoord = vec2(a_maskTexCoord.y + uOffset * a_maskTexCoord.z, a_maskTexCoord.w);
  v_halfLength = vec2(sign(a_normal.z) * halfWidth, abs(a_normal.z));
  vec4 pos = vec4(transformedAxisPos, a_position.z, 1.0) * projection;

  gl_Position = applyPivotTransform(pos, pivotTransform, 0.0);
}
