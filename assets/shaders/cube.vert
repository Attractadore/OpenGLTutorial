#version 330 core
layout (location = 0) in vec3 vPos;

out vec3 fPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    fPos = vPos;
    gl_Position = (projection * view * vec4(vPos, 1.0f)).xyww;
}
