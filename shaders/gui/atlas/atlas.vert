#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

layout (location = 0) out vec2 pass_uv;

layout (set = 1, binding = 0) uniform Matrices {
    mat4 projectMatrix;
} matrices;

layout ( push_constant ) uniform Push {
    mat4 transformMatrix;
    vec2 uv_offset;
    vec2 uv_size;
} push;

void main() {
    gl_Position = matrices.projectMatrix * push.transformMatrix * vec4(position, 1.0);
    pass_uv = push.uv_offset+uv*push.uv_size;
}