#ifdef ENABLE_VTF
in LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
in vec2 v_colorTexCoord;
#endif
uniform float u_opacity;

in float v_lengthY;

const float kAntialiasingThreshold = 0.92;

out vec4 v_FragColor;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_lengthY)));

  v_FragColor = color;
}
