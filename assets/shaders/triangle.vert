#version 330 core
layout (location = 0) in vec2 vPos;
layout (location = 1) in vec3 vColor;

out vec3 fColor;

uniform float time;

void main()
{
    mat2 rotation = mat2(
         cos(time), sin(time),
        -sin(time), cos(time)
    );
    fColor = vColor;
    gl_Position = vec4(rotation * normalize(vPos) * .75f, 0.0, 1.0);
}