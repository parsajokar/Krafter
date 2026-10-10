#version 450

layout(push_constant) uniform Push {
    mat4 viewProjection;
    vec3 blockPosition;
} pc;

layout(location = 0) in vec3 a_Position;

void main()
{
    const float margin = 0.002;
    vec3 world = pc.blockPosition - vec3(margin) + a_Position * (1.0 + 2.0 * margin);
    gl_Position = pc.viewProjection * vec4(world, 1.0);
}
