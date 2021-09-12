#version 450

layout (location = 0) in vec3 position;

layout (set = 0, binding = 0) uniform Matrices {
    mat4 projectMatrix;
} matrices;

layout ( push_constant ) uniform Push {
    mat4 transformMatrix;
} push;

void main() {
    gl_Position = matrices.projectMatrix * push.transformMatrix * vec4(position, 1.0);
}