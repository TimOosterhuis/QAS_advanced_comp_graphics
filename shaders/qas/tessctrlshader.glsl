#version 410
layout(vertices = 6) out;

in vec3 inputPatch[];
in vec3 inputPatchNormals[];

out vec3 controlNet[];
out vec3 controlNetNormals[];

uniform float tess_level_inner;
uniform float tess_level_outer;

#define i gl_InvocationID

void main()
{
    // Pass patch along to the tessellator
    controlNet[i] = inputPatch[i];
    controlNetNormals[i] = inputPatchNormals[i];
    
    // Set inner and outer tessellation levels
    if(i == 0)
    {
        // Inner tesselation level
        gl_TessLevelInner[0] = tess_level_inner;
        
        // Outer tesselation level
        gl_TessLevelOuter[0] = tess_level_outer;
        gl_TessLevelOuter[1] = tess_level_outer;
        gl_TessLevelOuter[2] = tess_level_outer;
    }
}

