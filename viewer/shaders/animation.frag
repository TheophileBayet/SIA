#version 410

uniform float lightIntensity;
uniform bool blinnPhong;
uniform float shininess;
uniform vec2 eta;
uniform sampler2D shadowMap;

in vec4 eyeVector;
in vec4 lightVector;
in vec4 vertColor;
in vec4 vertNormal;
in vec4 lightSpace;

out vec4 fragColor;

#define M_PI 3.1415926535897932384626433832795


/*
main puts the good value in fragColor
*/
void main( void )
{
  fragColor=vertColor;
}
