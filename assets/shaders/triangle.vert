#version 330 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;

out vec3 fPos;
out vec3 fNormal;

uniform mat4 model, view, projection;
uniform mat3 normal;

void main()
{
    fPos = vec3(model * vec4(vPos, 1.0f));
    fNormal = normal * vNormal;
    gl_Position = projection * view * model * vec4(vPos, 1.0f);
}