#version 410
layout(vertices = 6) out;

in vec3 inputPatch[];
in vec3 inputPatchNormals[];
//in float inputPatchCurvature[];

out vec3 controlNet[];
out vec3 controlNetNormals[];
out float controlNetCurvature[];

uniform float tess_level_inner;
uniform float tess_level_outer;
uniform float tess_level_min;
uniform float tess_level_max;

uniform mat4 modelviewmatrix;
uniform mat4 projectionmatrix;
uniform mat3 normalmatrix;

uniform int adaptive_tessellation;
uniform float zoom_tess;
uniform float curv_tess;
uniform float norm_tess;

float[6] calcCurvature(vec3 p0, vec3 p1, vec3 p2, vec3 n0, vec3 n1, vec3 n2)
{
    // Note: we cannot use edge-points because they are different for each patch, so then we can't easily prevent tessellation cracks
    float n0_len = length(n0);
    float n1_len = length(n1);
    float n2_len = length(n2);
    
    // calculate difference between normals along edges of hexagon
    float c01 = acos(dot(n0, n1) / n0_len / n1_len);
    float c12 = acos(dot(n1, n2) / n1_len / n2_len);
    float c20 = acos(dot(n2, n0) / n2_len / n0_len);
    
    // curvature at point
    float pcurvature[6];
    
    pcurvature[0] = c01;
    pcurvature[2] = c12;
    pcurvature[4] = c20;
    
    pcurvature[1] = (pcurvature[0] + pcurvature[2]) / 2.0;
    pcurvature[3] = (pcurvature[2] + pcurvature[4]) / 2.0;
    pcurvature[5] = (pcurvature[4] + pcurvature[0]) / 2.0;
    
    return pcurvature;
}

#define i gl_InvocationID

bool curvatureCalculated = false;
float curvature[6];

void main()
{
    // Note: the performance here can still be improved
    // TODO: Don't use 64 tessellation triangles if zoom level is not close enough (so we need to calculate pixel-width and see if tessellated triangle will be larger than a few pixels)
    //       This can be done by simply changing the zoom to an overlapping merger (instead of just multiplying, just let ZOOM regulate the MAX_TESS and MIN_TESS -> 1.0 -> 64.0)
    //       The zoom will then change the MIN_TESS and MAX_TESS, at which the other multipliers work on (remove clamp but normalize to 0.0 -> 1.0)
    //       Will do this for the report maybe (much more neater)
    
    if(!curvatureCalculated)
    {
        curvature = calcCurvature(inputPatch[0], inputPatch[2], inputPatch[4], inputPatchNormals[0], inputPatchNormals[2], inputPatchNormals[4]);
        curvatureCalculated = true;
    }
    
    // Pass patch along to the tessellator
    controlNet[i] = inputPatch[i];
    controlNetNormals[i] = inputPatchNormals[i];
    controlNetCurvature[i] = curvature[i];
    
    
    // Set inner and outer tessellation levels
    if(i == 0)
    {
        if(adaptive_tessellation == 1)
        {
            float MIN_VALUE = 0.000001;
            
            // Calculate vertices, normals, and actual positions
            // We cannot use edge-points because they are different for adjacent hexagons
            vec3 v0 = inputPatch[0];
            vec3 v1 = inputPatch[2];
            vec3 v2 = inputPatch[4];
            vec3 n0 = normalmatrix * inputPatchNormals[0];
            vec3 n1 = normalmatrix * inputPatchNormals[2];
            vec3 n2 = normalmatrix * inputPatchNormals[4];
            vec4 p0_mv = modelviewmatrix * vec4(v0, 1.0);
            vec4 p1_mv = modelviewmatrix * vec4(v1, 1.0);
            vec4 p2_mv = modelviewmatrix * vec4(v2, 1.0);
            vec4 p0 = projectionmatrix * p0_mv;
            vec4 p1 = projectionmatrix * p1_mv;
            vec4 p2 = projectionmatrix * p2_mv;
            
            float len01 = distance(v0, v1);
            float len12 = distance(v1, v2);
            float len20 = distance(v2, v0);
            
            float zoomPower = zoom_tess > 0.0 ? 1.0 : 0.0;
            float normPower = norm_tess > 0.0 ? 1.0 : 0.0;
            float curvPower = curv_tess > 0.0 ? 1.0 : 0.0;
            
            // Calculate tessellation by looking at distance
            float zoom = atan(1.0 / projectionmatrix[1][1]);
            float cam_dist = zoom * zoom;
            float t0_dist = 1.0 / max(MIN_VALUE, pow(1.0 / zoom_tess * p0.w * cam_dist, zoomPower));
            float t1_dist = 1.0 / max(MIN_VALUE, pow(1.0 / zoom_tess * p1.w * cam_dist, zoomPower));
            float t2_dist = 1.0 / max(MIN_VALUE, pow(1.0 / zoom_tess * p2.w * cam_dist, zoomPower));
            
            // Calculate tessellation by looking at perpendicularity of normal to camera-rays
            // Assuming that camera is at (0,0,0)
            vec3 camerapos = vec3(0.0);
            float t0_norm = 1.0 / max(MIN_VALUE, pow(1.0 / norm_tess * abs(dot(n0, camerapos - p0_mv.xyz)), normPower));
            float t1_norm = 1.0 / max(MIN_VALUE, pow(1.0 / norm_tess * abs(dot(n1, camerapos - p1_mv.xyz)), normPower));
            float t2_norm = 1.0 / max(MIN_VALUE, pow(1.0 / norm_tess * abs(dot(n2, camerapos - p2_mv.xyz)), normPower));
            
            // Calculate tessellation based on curvature
            float t01_curv = curv_tess > 0.0 ? pow(curv_tess * curvature[0], curvPower) : 1.0;
            float t12_curv = curv_tess > 0.0 ? pow(curv_tess * curvature[2], curvPower) : 1.0;
            float t20_curv = curv_tess > 0.0 ? pow(curv_tess * curvature[4], curvPower) : 1.0;
            
            // Calculate tessellation based on whether is facing camera (only if curvature is low we can do this extreme!)
            // If high curvature, don't do anything, but if low curvature, we can easily set this factor to 0.0 even!
            //float t0_front = dot(normalize(n0), normalize(camerapos - p0_mv.xyz));
            //t0_front = t0_front >= 0.0 ? 1.0 : max(0.2, min(1.0, pow(abs(1.0 / t0_front), 2.0)));
            //float t1_front = dot(normalize(n1), normalize(camerapos - p1_mv.xyz));
            //t1_front = t1_front >= 0.0 ? 1.0 : max(0.2, min(1.0, pow(abs(1.0 / t1_front), 2.0)));
            //float t2_front = dot(normalize(n2), normalize(camerapos - p2_mv.xyz));
            //t2_front = t2_front >= 0.0 ? 1.0 : max(0.2, min(1.0, pow(abs(1.0 / t2_front), 2.0)));
            float t0_front = 1.0;
            float t1_front = 1.0;
            float t2_front = 1.0;
            
            // Combine tessellation values
            float t0_total = t0_norm * t0_dist * t0_front;
            float t1_total = t1_norm * t1_dist * t1_front;
            float t2_total = t2_norm * t2_dist * t2_front;
            
            // Calculate semi-final tessellation values per vertex
            float multiplier = tess_level_inner;
            float t0 = clamp(multiplier * t0_total, 1.0, 64.0);
            float t1 = clamp(multiplier * t1_total, 1.0, 64.0);
            float t2 = clamp(multiplier * t2_total, 1.0, 64.0);
            
            // Calculate new max tess-level so that tessellation is never smaller than a pixel
            // TODO: This is just an approximation, and should depend on resolution (so we have to use the resolution of the OpenGL canvas in this calculation)
            float maxTess01 = tess_level_max == 1 ? min(64.0, 32.0 * len01 / zoom) : tess_level_max;
            float maxTess12 = tess_level_max == 1 ? min(64.0, 32.0 * len12 / zoom) : tess_level_max;
            float maxTess20 = tess_level_max == 1 ? min(64.0, 32.0 * len20 / zoom) : tess_level_max;
            
            // Calculate new min tess-level so that minimum tessellation is scaled down when greater camera distance
            float minTess01 = tess_level_min / zoom / 2.0;
            float minTess12 = tess_level_min / zoom / 2.0;
            float minTess20 = tess_level_min / zoom / 2.0;
            
            // Calculate final tessellation values per edge (multiply by curvature here because curvature is calculated for each edge instead of vertex)
            float t_01 = clamp(min(t0, t1) * t01_curv, minTess01, maxTess01);
            float t_12 = clamp(min(t1, t2) * t12_curv, minTess12, maxTess12);
            float t_20 = clamp(min(t2, t0) * t20_curv, minTess20, maxTess20);
            
            // Set inner tesselation level to minimum edge value
            gl_TessLevelInner[0] = min(min(t_01, t_12), t_20);
            
            // Set outer tesselation level for each edge
            gl_TessLevelOuter[0] = t_20;
            gl_TessLevelOuter[1] = t_01 + 1;
            gl_TessLevelOuter[2] = t_12;
        }
        else
        {
            // Inner tesselation level
            gl_TessLevelInner[0] = tess_level_inner;
            
            // Outer tesselation level
            gl_TessLevelOuter[0] = tess_level_outer;
            gl_TessLevelOuter[1] = tess_level_outer;
            gl_TessLevelOuter[2] = tess_level_outer;
        }
    }
}

