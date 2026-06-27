#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float zoom;
    vec2 pan;
    vec2 canvas;
    vec3 dotColor;
};
void main() {
    float dotSpacing = 35;
    float dotSize = 2;
    float dotSmoothing = 0.05;

    dotSize /= zoom;
    dotSmoothing *= zoom;
    dotSpacing /= exp2(round(log2(zoom)));
    vec2 canvasCoord = (qt_TexCoord0*canvas-pan)/zoom;
    vec2 dotPos = {round(canvasCoord.x/dotSpacing)*dotSpacing, round(canvasCoord.y/dotSpacing)*dotSpacing};
    float opacity = smoothstep(1/dotSize-dotSmoothing/2, 1/dotSize+dotSmoothing/2, 1/distance(canvasCoord, dotPos));
    opacity *= qt_Opacity;
    opacity = clamp(opacity, 0, 1);
    fragColor = vec4(dotColor, 1.0) * opacity;
}
