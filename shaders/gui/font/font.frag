#version 450

layout (location = 0) in vec2 pass_uv;
layout (location = 1) in vec4 pass_color;

layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 0) uniform sampler2D texture0;

void main() {
    vec4 texColor = texture(texture0, pass_uv);
    vec4 outColor = vec4(0, 0, 0, 0);
    /*if(texColor.a > 0.2)*/ outColor = vec4(pass_color.xyz, pass_color.w*texColor.r);
    fragColor = outColor;
}
