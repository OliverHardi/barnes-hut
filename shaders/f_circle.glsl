#version 330 core

in vec2 vUV;

out vec4 fragColor;

void main() {
    float len = length(vUV);
    if(len > 1.0){
        discard;
    }
    fragColor = vec4(0.85, 0.87, 0.9, .0);
}