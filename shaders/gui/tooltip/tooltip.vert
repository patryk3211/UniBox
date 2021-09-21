#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 pass_uv;
layout (location = 1) out vec2 pass_pos;
layout (location = 2) out vec2 pass_scale;

layout (set = 1, binding = 0) uniform Matrices {
    mat4 projectMatrix;
} matrices;

layout ( push_constant ) uniform Push {
    mat4 transformMatrix;
} push;

void main() {
    vec2 scale = vec2(length(push.transformMatrix[0].xyz), length(push.transformMatrix[1].xyz));
    mat4 scaleMatrix = mat4(
        vec4(scale.x, 0, 0, 0),
        vec4(0, scale.y, 0, 0),
        vec4(0, 0, 1, 0),
        vec4(0, 0, 0, 1)
    );

    gl_Position = matrices.projectMatrix * push.transformMatrix * vec4(position, 1.0);

    pass_uv = uv;
    pass_pos = (scaleMatrix * vec4(position, 1.0)).xy;
    pass_scale = scale;
}