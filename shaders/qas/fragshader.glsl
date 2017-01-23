#version 410
// Fragment shader
in vec3 tePosition;
in vec3 new_normal;
in float fCurvature;

uniform bool show_isophotes;
uniform int isophote_size;
uniform int curvature_mode;

uniform mat4 projectionmatrix;

out vec4 fColor;

void main()
{

    vec3 normal;
    normal = normalize(new_normal);

    vec3 compcolour;
    float alpha = 1.0;

    if (show_isophotes)
    {
        //initialize an arbitrary vector to compare with the normal at each pixel
        vec3 isophote_vec = vec3(1.0, 0.0, 0.0);
        //Look at the angle of the isophote vector and the normal (scaled with the isopote_size value) if it rounds to an even number
        //pick one color, if it rounds to an odd number pick the other color.
        //This is done to ensure an even spread of all the potential normal orientations between the two colors.
        //The standard use of the modulo of an angle in rounded degrees and 2 already gives very high frequency isophotes, hence the use of a
        //thickness or size scalar instead of a frequency scalar.
        if (mod(floor(degrees(acos(dot(normalize(isophote_vec), normal)))/isophote_size),2) == 0) {
            compcolour = vec3(0.0,0.0,1.0); //blue
        } else {
            compcolour = vec3(1.0,0.0,0.0); //red
        }

    }
    else
    {
        //phong

        vec3 lightpos = vec3(3.0, 0.0, 2.0);
        vec3 lightcolour = vec3(1.0);

        vec3 matcolour = vec3(0.53, 0.80, 0.87);
        vec3 matspeccolour = vec3(1.0);

        float matambientcoeff = 0.2;
        float matdiffusecoeff = 0.6;
        float matspecularcoeff = 0.4;

        vec3 surftolight = normalize(lightpos - tePosition);
        float diffusecoeff = max(0.0, dot(surftolight, normal));

        vec3 camerapos = vec3(0.0);
        vec3 surftocamera = normalize(camerapos - tePosition);
        vec3 reflected = 2 * diffusecoeff * normal - surftolight;
        float specularcoeff = max(0.0, dot(reflected, surftocamera));

        compcolour = min(1.0, matambientcoeff + matdiffusecoeff * diffusecoeff) * lightcolour * matcolour;
        compcolour += matspecularcoeff * specularcoeff * lightcolour * matspeccolour;
        
        // Overlay curvature
        if(curvature_mode > 0)
        {
            int mode = 2; // 1 -> override, 2 -> mix curvature
            if(fCurvature >= 0.0)
            {
                compcolour = vec3(fCurvature, 0.0, 0.0) + (mode - 1) * compcolour * (1 - 0.5 * fCurvature);
            }
            else
            {
                compcolour = vec3(0.0, 0.0, -fCurvature) + (mode - 1) * compcolour * (1 + 0.5 * fCurvature);
            }
        }
        
        // use gl_FragCoord to implement fog
    }

    fColor = vec4(compcolour, alpha);
}
