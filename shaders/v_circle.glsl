#version 330 core

layout (location = 0) in vec2 aCenter;
layout (location = 1) in vec2 aVelocity;
layout (location = 2) in float mass;

out vec2 vUV;

uniform float uRadius;
uniform vec2 uCameraPos;
uniform float uZoom;

void main() {
    int corner = gl_VertexID % 4;
    vec2 quadCorner = vec2(
        (corner == 1 || corner == 3) ? 1.0 : -1.0,
        (corner >= 2) ? 1.0 : -1.0
    );
    vUV = quadCorner;

    vec2 worldPos = aCenter + quadCorner * uRadius;
    gl_Position = vec4((worldPos-uCameraPos)*uZoom, 0.0, 1.0);
}