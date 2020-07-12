#version 330 core
layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VERT_OUT {
    vec4 normalRoot;
    vec4 normalEnd;
} gIn[];

void emitNormal(int index){
    gl_Position = gIn[index].normalRoot;
    EmitVertex();
    gl_Position = gIn[index].normalEnd;
    EmitVertex();
    EndPrimitive();
}

void main(){
    for (int i = 0; i < 3; i++){
        emitNormal(i);
    }
}