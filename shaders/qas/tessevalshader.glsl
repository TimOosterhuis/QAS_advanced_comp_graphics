#version 410
layout(triangles, equal_spacing, ccw) in;

in vec3 controlNet[];
in vec3 controlNetNormals[];

out vec3 tePosition;
out vec3 new_normal;

uniform mat4 modelviewmatrix;
uniform mat4 projectionmatrix;
uniform mat3 normalmatrix;

//calculate edge control point of a Bezier curve given the two corner control points and a midpoint
vec3 edgePoint(vec3 e, vec3 p0, vec3 p1)
{
    return ((e * 4.0 - p0 - p1) * 0.5);
}

// For simple triangle interpolation - currently unused
vec3 interpolate(vec3 v0, vec3 v1, vec3 v2)
{
    // Interpolate between three points
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

//calculate position on a quadratic bezier patch given barycentric coordinates and control net
vec3 getBezierPos(float u, float v, float w, vec3 p0, vec3 p1, vec3 p2, vec3 e0, vec3 e1, vec3 e2)
{
    //triangular Bezier basisfunctions
    vec3 bezierPos =  w * (p0 * w + edgePoint(e0, p0, p1) * 2 * u) +
            u * (p1 * u + edgePoint(e1, p1, p2) * 2 * v) +
            v * (p2 * v + edgePoint(e2, p0, p2) * 2 * w);
    return bezierPos;

}

void main()
{ 
    // Get barycentric coordinates
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;
    float w = gl_TessCoord.z; // = 1.0 - u - v
    
    //get the position on the patch
    tePosition = getBezierPos(u, v, w, controlNet[0], controlNet[2], controlNet[4], controlNet[1], controlNet[3], controlNet[5]);
    
    //get the corresponding normal using the quadratic bezier patch
    new_normal = getBezierPos(u, v, w, controlNetNormals[0], controlNetNormals[2], controlNetNormals[4], controlNetNormals[1], controlNetNormals[3], controlNetNormals[5]);
    new_normal = normalize(normalmatrix * new_normal);

    //update global position and tePosition
    gl_Position = projectionmatrix * modelviewmatrix * vec4(tePosition, 1);
    tePosition = vec3(modelviewmatrix * vec4(tePosition, 1.0));


}
