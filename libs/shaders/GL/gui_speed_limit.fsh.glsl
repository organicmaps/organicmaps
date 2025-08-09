layout (location = 0) in vec2 v_position;

layout (binding = 0) uniform UBO
{
    mat4 u_modelView;
    mat4 u_projection;

    vec3 u_color;
    vec3 u_outlineColor;
    vec3 u_edgeColor;

    float u_length;
    float u_outlineWidthRatio;
    float u_edgeWidthRatio;
};

layout (location = 0) out vec4 v_FragColor;

void renderCircle()
{
    float R = 1.0;
    float R2 = R - u_outlineWidthRatio;
    float dist = length(v_position);
    float aa = fwidth(dist);// Antialiasing width

    // Outer edge antialiasing
    float outerAlpha = 1.0 - smoothstep(R - aa, R + aa, dist);

    // Inner edge antialiasing (outline to fill)
    float innerAlpha = smoothstep(R2 - aa, R2 + aa, dist);

    vec3 color = mix(u_color, u_outlineColor, innerAlpha);
    float alpha = outerAlpha;

    v_FragColor = vec4(color, alpha);
}

// Signed distance function for rounded rect
float roundedBoxSDF(vec2 p, vec2 halfSize, float radius) {
    vec2 q = abs(p) - halfSize + vec2(radius);
    return length(max(q, 0.0)) - radius;
}

// Returns alpha-masked color for one layer
vec4 drawLayer(float dist, float bandStart, float bandEnd, vec3 color, float aa) {
    float alpha = smoothstep(bandEnd + aa, bandEnd - aa, dist) * (1.0 - smoothstep(bandStart + aa, bandStart - aa, dist));
    return vec4(color, alpha);
}

void renderGaplessRoundedRect() {
    vec2 halfSize = vec2(1.0);// full square shape
    float radius = 0.25;
    float dist = roundedBoxSDF(v_position, halfSize, radius);
    float aa = fwidth(dist);// prevent div/zero

    float thinLine = u_edgeWidthRatio;
    float outline  = u_outlineWidthRatio;

    float outer     = 0.0;
    float outerLine = -thinLine;
    float outlineIn = -outline;

    vec4 c_outerLine = drawLayer(dist, outerLine, outer, u_edgeColor, aa);
    vec4 c_outline   = drawLayer(dist, outlineIn, outerLine, u_outlineColor, aa);
    vec4 c_fill      = drawLayer(dist, -1.5, outlineIn, u_color, aa);

    vec3 color = c_outerLine.rgb * c_outerLine.a + c_outline.rgb * c_outline.a + c_fill.rgb * c_fill.a;
    float alpha = c_outerLine.a + c_outline.a + c_fill.a;

    // Ограничим альфу максимумом 1.0
    alpha = clamp(alpha, 0.0, 1.0);

    // Premultiplied alpha: умножаем цвет на альфу
    color = color / max(alpha, 0.0001);// Чтобы избежать деления на 0

    v_FragColor = vec4(color, alpha);
}

void main()
{
    renderCircle();
    //    renderGaplessRoundedRect();
}