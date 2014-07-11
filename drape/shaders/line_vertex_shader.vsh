attribute highp vec4 position;
attribute highp vec4 deltas;
attribute highp vec4 width_type;
attribute highp vec4 centres;
attribute highp vec4 color1;
attribute highp vec4 color2;

varying highp float v_dx;
varying highp vec4 v_radius;
varying highp vec4 v_centres;
varying highp vec2 v_type;

varying highp vec4 baseColor;
varying highp vec4 outlineColor;

uniform highp mat4 modelView;
uniform highp mat4 projection;

void main(void)
{
  float r = abs(width_type.x);
  vec2 dir = position.zw - position.xy;
  float len = length(dir);
  vec4 pos2 = vec4(position.xy, deltas.z, 1) * modelView;
  vec4 direc = vec4(position.zw, deltas.z, 1) * modelView;
  dir = direc.xy - pos2.xy;
  float l2 = length(dir);
  dir = normalize(dir);
  dir *= len * r;
  pos2 += vec4(dir, 0, 0);

  gl_Position = pos2 * projection;

  v_dx = (deltas.y + deltas.x * r / l2 * len);
  v_radius.x = width_type.x;
  v_radius.y = r;
  v_radius.w = width_type.w;
  vec2 centr1 = (vec4(centres.xy, 0, 1) * modelView).xy;
  vec2 centr2 = (vec4(centres.zw, 0, 1) * modelView).xy;
  float len2 = length(centr1 - centr2);
  v_radius.z = len2;
  v_centres.xy = centr1;
  v_centres.zw = centr2;
  v_type = width_type.yz;

  baseColor = color1;
  outlineColor = color2;
}
