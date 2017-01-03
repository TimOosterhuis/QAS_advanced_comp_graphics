#version 410
// Fragment shader
in vec3 tePosition;
in vec3 new_normal;

out vec4 fColor;

void main()
{
    //phong
    vec3 lightpos = vec3(3.0, 0.0, 2.0);
    vec3 lightcolour = vec3(1.0);
    
    vec3 matcolour = vec3(0.53, 0.80, 0.87);
    vec3 matspeccolour = vec3(1.0);
    
    float matambientcoeff = 0.2;
    float matdiffusecoeff = 0.6;
    float matspecularcoeff = 0.4;
    
    vec3 normal;
    //normals point inward at this point, and are therefore inverted
    normal = normalize(-new_normal);
    
    vec3 surftolight = normalize(lightpos - tePosition);
    float diffusecoeff = max(0.0, dot(surftolight, normal));
    
    vec3 camerapos = vec3(0.0);
    vec3 surftocamera = normalize(camerapos - tePosition);
    vec3 reflected = 2 * diffusecoeff * normal - surftolight;
    float specularcoeff = max(0.0, dot(reflected, surftocamera));
    
    vec3 compcolour = min(1.0, matambientcoeff + matdiffusecoeff * diffusecoeff) * lightcolour * matcolour;
         compcolour += matspecularcoeff * specularcoeff * lightcolour * matspeccolour;
    
    fColor = vec4(compcolour, 1.0);
}
