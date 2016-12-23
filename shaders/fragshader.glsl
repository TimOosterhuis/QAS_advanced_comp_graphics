#version 410
// Fragment shader

layout (location = 0) in vec3 vertcoords_camera_fs;
layout (location = 1) in vec3 vertnormal_camera_fs;

out vec4 fColor;

uniform int isophote_size; // thickness of the isophote lines
uniform bool show_isophotes; // isophote lines toggle

void main() {
  //calculate normal
  vec3 normal;
  normal = normalize(vertnormal_camera_fs);

  vec3 compcolour;

  //if isophote lines are enabled
  if (show_isophotes) {
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

  }  else {
      // do phong shading
      vec3 lightpos = vec3(3.0, 0.0, 2.0);

      vec3 lightcolour = vec3(1.0,1.0,1.0);

      vec3 matcolour = vec3(0.53, 0.80, 0.87);
      vec3 matspeccolour = vec3(1.0);

      float matambientcoeff = 0.2;
      float matdiffusecoeff = 0.6;
      float matspecularcoeff = 0.4;

      vec3 surftolight = normalize(lightpos - vertcoords_camera_fs);

      float diffusecoeff = max(0.0, dot(surftolight, normal));

      vec3 camerapos = vec3(0.0);
      vec3 surftocamera = normalize(camerapos - vertcoords_camera_fs);
      vec3 reflected = 2 * diffusecoeff * normal - surftolight;
      float specularcoeff = max(0.0, dot(reflected, surftocamera));

      compcolour = min(1.0, matambientcoeff + matdiffusecoeff * diffusecoeff) * lightcolour * matcolour;
      compcolour += matspecularcoeff * specularcoeff * lightcolour * matspeccolour;

   }
   //return final color
   fColor = vec4(compcolour, 1.0);
}
