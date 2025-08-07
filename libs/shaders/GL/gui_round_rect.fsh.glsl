layout (location = 0) in vec2 v_position;

layout (location = 0) out vec4 v_FragColor;

layout (binding = 0) uniform UBO
{
    mat4 u_modelView;
    mat4 u_projection;

    vec2 u_colorTexCoords;
    vec2 u_outlineColorTexCoords;
    vec2 u_edgeColorTexCoords;

    float u_length;
    float u_radius;
    float u_outlineWidthRatio;
    float u_edgeWidthRatio;
};

layout (binding = 1) uniform sampler2D u_colorTex;

// Signed distance function for rounded rect
float roundedBoxSDF(vec2 p, vec2 halfSize, float radius) {
    vec2 q = abs(p) - halfSize + vec2(radius);
    return length(max(q, 0.0)) - radius;
}

// Returns alpha-masked color for one layer
vec4 drawLayer(float dist, float bandStart, float bandEnd, vec4 color, float aa) {
    float alpha = smoothstep(bandEnd + aa, bandEnd - aa, dist) * (1.0 - smoothstep(bandStart + aa, bandStart - aa, dist));
    return vec4(color.rgb, alpha);
}

void main()
{
    vec4 color = texture(u_colorTex, u_colorTexCoords);
    vec4 outlineColor = texture(u_colorTex, u_outlineColorTexCoords);
    vec4 edgeColor = texture(u_colorTex, u_edgeColorTexCoords);

    vec2 halfSize = vec2(1.0);
    float dist = roundedBoxSDF(v_position, halfSize, u_radius);
    float aa = fwidth(dist);

    float thinLine = u_edgeWidthRatio;
    float outline  = u_outlineWidthRatio;

    float outer     = 0.0;
    float outerLine = -thinLine;
    float outlineIn = -outline;

    vec4 c_outerLine = drawLayer(dist, outerLine, outer, edgeColor, aa);
    vec4 c_outline   = drawLayer(dist, outlineIn, outerLine, outlineColor, aa);
    vec4 c_fill      = drawLayer(dist, -1.5, outlineIn, color, aa);

    vec3 finalColor = c_outerLine.rgb * c_outerLine.a + c_outline.rgb * c_outline.a + c_fill.rgb * c_fill.a;
    float alpha = c_outerLine.a + c_outline.a + c_fill.a;

    alpha = clamp(alpha, 0.0001, 1.0);
    finalColor = finalColor / alpha;
    v_FragColor = vec4(finalColor, alpha);
}