#version 450

layout (location = 0) in vec4 position;
layout (location = 1) in vec4 vertexColor;

layout (set = 0, binding = 0) uniform GlobalMatricies {
    mat4 viewMatrix;
    mat4 projectMatrix;
} globalMatricies;

/*layout ( push_constant ) uniform PushData {
    mat4 modelMatrix;
} pushData;*/

layout (location = 0) out vec4 pass_color;

void main() {
    gl_Position = globalMatricies.projectMatrix * globalMatricies.viewMatrix * position;

    pass_color = vertexColor;
}