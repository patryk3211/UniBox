#version 450

layout (location = 0) in vec2 pass_uv;
layout (location = 1) in vec2 pass_pos;
layout (location = 2) in vec2 pass_scale;

layout (location = 0) out vec4 fragColor;

layout (set = 0, binding = 0) uniform sampler2D texture0;

const float imageSize = 16.0;
const float edgeThickness = 6.0;

const float offset1 = ((edgeThickness+0.5)/imageSize);
const float offset2 = ((imageSize-(edgeThickness+0.5))/imageSize);

void main() {
    vec2 edgePos = pass_scale/2;
    vec2 absPos = pass_pos+edgePos;
    vec2 scaleRatio = pass_scale/imageSize;

    bool corner =
    (pass_pos.x < -edgePos.x+edgeThickness && pass_pos.y < -edgePos.y+edgeThickness) ||
    (pass_pos.x >  edgePos.x-edgeThickness && pass_pos.y >  edgePos.y-edgeThickness) ||
    (pass_pos.x < -edgePos.x+edgeThickness && pass_pos.y >  edgePos.y-edgeThickness) ||
    (pass_pos.x >  edgePos.x-edgeThickness && pass_pos.y < -edgePos.y+edgeThickness);

    bool edge =
    pass_pos.x <= -edgePos.x+edgeThickness ||
    pass_pos.y <= -edgePos.y+edgeThickness ||
    pass_pos.x >=  edgePos.x-edgeThickness ||
    pass_pos.y >=  edgePos.y-edgeThickness;

    vec2 genUv;
    if(corner) {
        const float xMin = absPos.x/edgeThickness*offset1;
        const float yMin = absPos.y/edgeThickness*offset1;

        const float xMax = offset2+(absPos.x-(pass_scale.x-edgeThickness))/edgeThickness*offset1;
        const float yMax = offset2+(absPos.y-(pass_scale.y-edgeThickness))/edgeThickness*offset1;
        genUv = vec2(mix(xMin, xMax, pass_pos.x > 0), mix(yMin, yMax, pass_pos.y > 0));
    } else if(edge) {
        const float xMov = offset1+(int(absPos.x-edgeThickness)%4)/imageSize;
        const float yMov = offset1+(int(absPos.y-edgeThickness)%4)/imageSize;

        const float xMin = absPos.x/edgeThickness*offset1;
        const float yMin = absPos.y/edgeThickness*offset1;

        const float xMax = offset2+(absPos.x-(pass_scale.x-edgeThickness))/edgeThickness*offset1;
        const float yMax = offset2+(absPos.y-(pass_scale.y-edgeThickness))/edgeThickness*offset1;

        bool verticalEdge = pass_pos.x <= -edgePos.x+edgeThickness || pass_pos.x >= edgePos.x-edgeThickness;
        bool horizontalEdge = pass_pos.y <= -edgePos.y+edgeThickness || pass_pos.y >= edgePos.y-edgeThickness;

        genUv = vec2(
            mix(mix(xMin, xMax, pass_pos.x > 0), xMov, horizontalEdge),
            mix(mix(yMin, yMax, pass_pos.y > 0), yMov, verticalEdge)
        );
    } else {
        const float xMov = offset1+(int(absPos.x-edgeThickness)%4)/imageSize;
        const float yMov = offset1+(int(absPos.y-edgeThickness)%4)/imageSize;
        genUv = vec2(xMov, yMov);
    }
    fragColor = texture(texture0, genUv);
}