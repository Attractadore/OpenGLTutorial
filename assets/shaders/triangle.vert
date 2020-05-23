#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec2 vTex;

out vec3 fColor;
out vec2 fTex;

uniform mat4 model, view, projection;

void main()
{
    fColor = vColor;
    fTex = vTex;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}