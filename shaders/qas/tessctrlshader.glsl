#version 410
layout(vertices = 6) out;

in vec3 inputPatch[];
in vec3 inputPatchNormals[];

out vec3 controlNet[];
out vec3 controlNetNormals[];

uniform float tess_level_inner;
uniform float tess_level_outer;

uniform mat4 modelviewmatrix;
uniform mat4 projectionmatrix;

uniform int adaptive_tessellation;

#define i gl_InvocationID

void main()
{
    // Pass patch along to the tessellator
    controlNet[i] = inputPatch[i];
    controlNetNormals[i] = inputPatchNormals[i];
    
    // Set inner and outer tessellation levels
    if(i == 0)
    {
        // Use the position from inputPatch[i] to calculate adaptive tessellation value based on distance to camera
        // but what if tessellated thingy is both close and far away? yeah, then we could just remesh into smaller parts
        // do a multiplier here for the tessellation level:
        float tessLevelInner = tess_level_inner;
        float tessLevelOuter = tess_level_outer;
        
        if(adaptive_tessellation == 1)
        {
            vec4 position = projectionmatrix * modelviewmatrix * vec4(inputPatch[i], 1);
            float distToCamera = position.w * position.w;
            if(distToCamera == 0.0)
            {
                distToCamera = 0.0000001;
            }
            tessLevelInner = clamp(tess_level_inner / distToCamera, 1.0, 64.0);
            tessLevelOuter = clamp(tess_level_outer / distToCamera, 1.0, 64.0);
            
            // make sure tess levels are power of 2 to prevent cracks
            tessLevelInner = pow(2, ceil(log(tessLevelInner)/log(2)));
            // we should make sure tess_level_outer is the same for same distances
            // not only do this, but also check other values for i
            // but for now, outer tess levels are just static:
            tessLevelOuter = clamp(tess_level_outer / 4.0, 1.0, 64.0);//pow(2, ceil(log(tessLevelOuter)/log(2)));
        }
        
        // Inner tesselation level
        gl_TessLevelInner[0] = tessLevelInner;
        
        // Outer tesselation level
        gl_TessLevelOuter[0] = tessLevelOuter;
        gl_TessLevelOuter[1] = tessLevelOuter;
        gl_TessLevelOuter[2] = tessLevelOuter;
    }
}

