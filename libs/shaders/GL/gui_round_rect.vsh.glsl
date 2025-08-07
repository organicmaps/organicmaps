layout (location = 0) in vec2 a_position;

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

layout (location = 0) out vec2 v_position;

void main()
{
    v_position = a_position;

    gl_Position = vec4(a_position * u_length, 0, 1) * u_modelView * u_projection;
    #ifdef VULKAN
    gl_Position.y = -gl_Position.y;
    gl_Position.z = (gl_Position.z  + gl_Position.w) * 0.5;
    #endif
}