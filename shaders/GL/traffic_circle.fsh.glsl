// Warning! Beware to use this shader. "discard" command may significally reduce performance.
// Unfortunately some CG algorithms cannot be implemented on OpenGL ES 2.0 without discarding
// fragments from depth buffer.

in vec2 v_colorTexCoord;
in vec3 v_radius;

uniform sampler2D u_colorTex;
uniform float u_opacity;

const float kAntialiasingThreshold = 0.92;

out vec4 v_FragColor;

void main()
{
  vec4 color = texture(u_colorTex, v_colorTexCoord);
  float smallRadius = v_radius.z * kAntialiasingThreshold;
  float stepValue = smoothstep(smallRadius * smallRadius, v_radius.z * v_radius.z,
                               v_radius.x * v_radius.x + v_radius.y * v_radius.y);
  color.a = u_opacity * (1.0 - stepValue);
  if (color.a < 0.01)
    discard;
  v_FragColor = color;
}
