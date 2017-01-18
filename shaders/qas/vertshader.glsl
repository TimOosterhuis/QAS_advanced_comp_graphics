#version 410
// Vertex shader

layout (location = 0) in vec3 positions;
layout (location = 1) in vec3 normals;
out vec3 inputPatch;
out vec3 inputPatchNormals;

void main()
{
    // Pass each vertex along the pipeline
    inputPatch = positions;
    inputPatchNormals = normals;
}
