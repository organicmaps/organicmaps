#ifdef ENABLE_VTF
varying LOW_P vec4 v_color;
#else
uniform sampler2D u_colorTex;
varying vec2 v_colorTexCoord;
#endif
uniform float u_opacity;

varying float v_lengthY;

const float kAntialiasingThreshold = 0.92;

void main()
{
#ifdef ENABLE_VTF
  LOW_P vec4 color = v_color;
#else
  LOW_P vec4 color = texture2D(u_colorTex, v_colorTexCoord);
#endif
  color.a *= u_opacity;
  color.a *= (1.0 - smoothstep(kAntialiasingThreshold, 1.0, abs(v_lengthY)));

  gl_FragColor = samsungGoogleNexusWorkaround(color);
}
