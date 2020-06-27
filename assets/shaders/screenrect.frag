#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D currentTexture, previousTexture;
uniform float cW, pW;
uniform bool bGreyScale, bTAA;

vec3 toGrey(vec3 color){
    float gs = 0.2126f * color.r + 0.7152f * color.g + 0.0722 * color.b;
    return vec3(gs);
}

void main(){
    vec3 color = vec3(texture(currentTexture, fTex));
    if (bTAA){
        vec3 previousColor = vec3(texture(previousTexture, fTex));
        color = mix(previousColor, color, cW / (cW + pW));
    }
    if (bGreyScale){
        vec3 gs = toGrey(color);
        color = mix(color, gs, 0.8f);
    }
    fColor = vec4(color, 1.0f);
}
