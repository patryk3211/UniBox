#version 450

layout (location = 0) in vec4 pass_color;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = pass_color;
}