#version 410
// Vertex shader

layout (location = 0) in vec3 vertcoords_world_vs;
out vec3 inputPatch;

void main() {
  //pass each vertex along the pipeline and do nothing else
  inputPatch = vertcoords_world_vs;
}
