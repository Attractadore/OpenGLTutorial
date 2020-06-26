#version 330 core
in vec2 fTex;

out vec4 fColor;

uniform sampler2D screenTexture;
uniform bool bGreyScale;

vec3 toGrey(vec3 color){
    float gs = 0.2126f * color.r + 0.7152f * color.g + 0.0722 * color.b;
    return vec3(gs);
}

void main(){
    vec3 color = vec3(texture(screenTexture, fTex));
    vec3 res;
    if (bGreyScale){
        vec3 gs = toGrey(color);
        res = mix(color, gs, 0.8f);
    }
    else {
        res = color;
    }
    fColor = vec4(res, 1.0f);
}
