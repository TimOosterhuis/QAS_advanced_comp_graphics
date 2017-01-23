#version 410
// Vertex shader

layout (location = 0) in vec3 positions;
layout (location = 1) in vec3 normals;
//layout (location = 2) in float curvature;

out vec3 inputPatch;
out vec3 inputPatchNormals;
//out float inputPatchCurvature;

void main()
{
    // Pass each vertex along the pipeline
    inputPatch = positions; // vertcoords_world_vs
    inputPatchNormals = normals;
    //inputPatchCurvature = curvature;
}
