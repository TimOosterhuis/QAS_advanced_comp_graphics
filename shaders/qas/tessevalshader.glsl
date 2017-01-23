#version 410
layout(triangles, equal_spacing, ccw) in;

in vec3 controlNet[];
in vec3 controlNetNormals[];
in float controlNetCurvature[];

out vec3 tePosition;
out vec3 new_normal;
out float fCurvature;

uniform mat4 modelviewmatrix;
uniform mat4 projectionmatrix;
uniform mat3 normalmatrix;

vec3 edgePoint(vec3 e, vec3 p0, vec3 p1)
{
    return ((e * 4.0 - p0 - p1) * 0.5);
}

// For simple triangle interpolation
vec3 interpolate(vec3 v0, vec3 v1, vec3 v2)
{
    // Interpolate between three points
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

vec3 getBezierPos(float u, float v, float w, vec3 p0, vec3 p1, vec3 p2, vec3 e0, vec3 e1, vec3 e2)
{
    // u,v,w | u,v,w | v,w,u changed into w,u,v | w,u,v | v,w,u as shown in listing from QAS paper
    //# listing from the QAS paper says w,u,v | w,u,v | u,v,w,. The important thing is that corner points and edge points get matched
    //# get matched with the same barycentric coordinates. (u,v,w | u,v,w | v,w,u would have worked fine too)
    vec3 bezierPos =  w * (p0 * w + edgePoint(e0, p0, p1) * 2 * u) +
            u * (p1 * u + edgePoint(e1, p1, p2) * 2 * v) +
            v * (p2 * v + edgePoint(e2, p0, p2) * 2 * w);
    return bezierPos;

}

// Note: TODO: Should make a generic function that accepts any float/vec2/vec3/vec4 (not sure if possible)
float edgePointFloat(float e, float p0, float p1)
{
    return ((e * 4.0 - p0 - p1) * 0.5);
}
float getBezierFloat(float u, float v, float w, float p0, float p1, float p2, float e0, float e1, float e2)
{
    return w * (p0 * w + edgePointFloat(e0, p0, p1) * 2 * u)
         + u * (p1 * u + edgePointFloat(e1, p1, p2) * 2 * v)
         + v * (p2 * v + edgePointFloat(e2, p0, p2) * 2 * w);

}

void main()
{ 
    // Get barycentric coordinates
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z; // = 1.0 - u - v
    
    // In order to uncomment this, we need to:
    // - first input 6 vertices in tessctrlshader
    // - and set GL_PATCH_VERTICES to 6 (instead of 3)
    // - and update the updateVertexArrayObjectQAS function so that it injects indices for both vertices and calculated edgepoints
    
    tePosition = getBezierPos(u, v, w, controlNet[0], controlNet[2], controlNet[4], controlNet[1], controlNet[3], controlNet[5]);
    
    //tePosition = interpolate(controlNet[0], controlNet[2], controlNet[4]);
    
    // Also, we'll need to calculate the correct normals
    new_normal = getBezierPos(u, v, w, controlNetNormals[0], controlNetNormals[2], controlNetNormals[4], controlNetNormals[1], controlNetNormals[3], controlNetNormals[5]);
    new_normal = normalize(normalmatrix * new_normal);
    
    // calculate correct curvature
    fCurvature = getBezierFloat(u, v, w, controlNetCurvature[0], controlNetCurvature[2], controlNetCurvature[4], controlNetCurvature[1], controlNetCurvature[3], controlNetCurvature[5]);
    
    gl_Position = projectionmatrix * modelviewmatrix * vec4(tePosition, 1);
    tePosition = vec3(modelviewmatrix * vec4(tePosition, 1.0));


}
