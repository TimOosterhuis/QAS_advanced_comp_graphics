#version 410
layout(vertices = 6) out;

in vec3 inputPatch[];
out vec3 controlNet[];

uniform float tess_level_inner;
uniform float tess_level_outer;

void main() {
    //pass patch along to the tessellator
    controlNet[gl_InvocationID] = inputPatch[gl_InvocationID];
    //set inner and outer tessellation level
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = 1;//tess_level_inner;
        gl_TessLevelOuter[0] = 1;//tess_level_outer;
        gl_TessLevelOuter[1] = 1;//tess_level_outer;
        gl_TessLevelOuter[2] = 1;//tess_level_outer;
    }
}

