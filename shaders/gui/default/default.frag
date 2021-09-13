#version 450

layout (location = 0) in vec2 pass_uv;

layout (location = 0) out vec4 fragColor;

layout (binding = 1) uniform sampler2D texture0;

void main() {
    fragColor = texture(texture0, pass_uv);
}