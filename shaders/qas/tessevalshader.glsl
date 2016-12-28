#version 410
layout(quads, equal_spacing, ccw) in;
in vec3 controlNet[];

out vec3 tePosition;
out vec3 new_normal;;
uniform mat4 modelviewmatrixTES;
uniform mat4 projectionmatrixTES;

void calcEdgePoint(vec3 e, vec3 p0, vec3 p1) {
    return ((e * 4 - p0 - p1) / 2);
}

void getBezierPos(float u, float v, float w, vec3 p0, vec3 p1, vec3 p2, vec3 e0, vec3 e1, vec3 e2) {
    vec3 p200 = p0, p020 = p1, p002 = p2;
    vec3 p110 = calcEdgePoint(e0, p0, p1);
    vec3 p101 = calcEdgePoint(e2, p0, p2);
    vec3 p011 = calcEdgePoint(e1, p1, p2);
    return u * (p200 * u + p110 * 2 * v) +
           v * (p020 * v + p011 * 2 * w) +
           w * (p002 * w + p101 * 2 * u);
}

void main()
{ 
    //get barycentric coordinates
    float u = gl_TessCoord.x, v = gl_TessCoord.y, w = TessCoord.z;
    tePosition = getBezierPos(u, v, w, controlNet[0], controlNet[1], controlNet[2], controlnet[3], controlNet[4], controlNet[5]);
    //



    gl_Position = projectionmatrixTES * modelviewmatrixTES * vec4(tePosition, 1);

}
