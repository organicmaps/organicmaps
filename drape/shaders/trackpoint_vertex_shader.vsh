attribute vec3 a_normal;
attribute vec3 a_position;
attribute vec4 a_color;

uniform mat4 modelView;
uniform mat4 projection;
uniform mat4 pivotTransform;

#ifdef SAMSUNG_GOOGLE_NEXUS
uniform sampler2D u_colorTex;
varying lowp vec4 v_fakeColor;
#endif

varying vec3 v_radius;
varying vec4 v_color;

void main(void)
{
  vec3 radius = a_normal * a_position.z;

  // Here we intentionally decrease precision of 'pos' calculation
  // to eliminate jittering effect in process of billboard reconstruction.
  lowp vec4 pos = vec4(a_position.xy, 0, 1) * modelView;
  highp vec4 shiftedPos = vec4(radius.xy, 0, 0) + pos;
  vec4 finalPos = shiftedPos * projection;
  float w = finalPos.w;
  finalPos.xyw = (pivotTransform * vec4(finalPos.xy, 0.0, w)).xyw;
  finalPos.z *= finalPos.w / w;
  gl_Position = finalPos;

  v_radius = radius;
  v_color = a_color;

#ifdef SAMSUNG_GOOGLE_NEXUS
  // Because of a bug in OpenGL driver on Samsung Google Nexus this workaround is here.
  v_fakeColor = texture2D(u_colorTex, vec2(0.0, 0.0));
#endif
}
