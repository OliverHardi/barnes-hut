#version 330 core
layout (location = 0) in vec2 aPos;

uniform vec2 uMin;
uniform vec2 uMax;

uniform vec2 uCameraPos;
uniform float uZoom;

out float vSpeed;

void main() {
    vec2 pos = mix(uMin, uMax, aPos);
    gl_Position = vec4((pos-uCameraPos)*uZoom, 0.0, 1.0);
}
