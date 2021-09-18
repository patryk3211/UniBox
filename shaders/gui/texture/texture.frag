#version 450

layout (location = 0) in vec2 pass_uv;
layout (location = 1) in vec4 pass_tint;

layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 0) uniform sampler2D texture0;

void main() {
    fragColor = texture(texture0, pass_uv) * pass_tint;
}