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
* This function calculates the ambiant component of the Blinn-Phong model for lighting
*/
vec4 ambiant(vec4 color, float ka, float I){
  return color*ka*I;
}

/*
*This function calculates max(V1.V2,0), where V1 and V2 are both vec4 vectors.
*/

float max_scalar_zero(vec4 V1, vec4 V2) {
  float res = max(dot(V1,V2),0);
  return res;
}

/*
* This function calculates the diffuse component of the Blinn-Phong model for lighting
*/
vec4 diffuse(vec4 color,vec4 normal,vec4 LightVector, float kd, float I){
  return color*max_scalar_zero(normal,LightVector)*kd*I;
}

/*
* this function calculates the Fresnel coefficient
*/
float fresnel_coeff(vec2 eta, float cost){
  vec2 eta2 = vec2(eta.x*eta.x - eta.y*eta.y, 2*eta.x*eta.y);
  vec2 tmp = eta2 + 1 - cost*cost;
  float x_ci = sqrt((tmp.x+sqrt(tmp.x*tmp.x+tmp.y+tmp.y))/2);
  float y_ci = tmp.y / (2*x_ci);
  vec2 ci = vec2(x_ci,y_ci);
  vec2 nume = cost - ci;
  vec2 denom = cost + ci;
  float Fs = abs(denom.x*denom.x + denom.y*denom.y);
  Fs = Fs*Fs;
  Fs = 1/Fs;
  float coef = nume.x*nume.x*denom.x*denom.x;
  coef += nume.y*nume.y*denom.y*denom.y;
  coef += nume.y*nume.y*denom.x*denom.x;
  coef += nume.x*nume.x*denom.y*denom.y;
  Fs = Fs * coef;
  nume = eta2*cost - ci;
  denom = eta2*cost + ci;
  float Fp = abs(denom.x*denom.x + denom.y*denom.y);
  Fp = Fp*Fp;
  Fp = 1/Fp;
  coef = nume.x*nume.x*denom.x*denom.x;
  coef += nume.y*nume.y*denom.y*denom.y;
  coef += nume.y*nume.y*denom.x*denom.x;
  coef += nume.x*nume.x*denom.y*denom.y;
  Fp = Fp * coef;
  return((Fs+Fp)/2);
 }

 // float fresnel_coeff(float eta, float cost){
 //     float ci = abs(eta*eta + 1 - cost*cost) ;
 //     ci = sqrt(ci);
 //     float nume = cost - ci;
 //     float denom = cost + ci;
 //     float Fs = abs(nume/denom);
 //     Fs = Fs*Fs;
 //     nume = eta*eta*cost - ci;
 //     denom = eta*eta*cost + ci;
 //     float Fp = abs(nume/denom);
 //     Fp = Fp * Fp;
 //     return((Fs+Fp)/2);
 //  }

/*
* this function calculates the specular component of the Blinn-Phong model for ligthing
*/
vec4 specular_blinn(vec2 eta, float cost, vec4 color, vec4 normal, vec4 H, float s, float I){
  float F = fresnel_coeff(eta,cost);
  float max = max_scalar_zero(normal,H);
  max = pow(max,s);


  return ( F*max* color *  I);
}

/*
* this function calculates the D coefficient
*/
float compute_D(float alpha, float cost){
    // 0 < theta < pi/2
    if (cost > 0){ // NE COUVRE PAS ENCORE TOUS LES CAS BIEN!!!
        float res = (alpha*alpha) / (M_PI * cost*cost*cost*cost);
        float tant_square = 1/(cost*cost) - 1;
        float denom = alpha*alpha + tant_square;
        res = res / (denom*denom);
        return res;
    } else {
        return 0;
    }
}

/*
* this function calculates the G1 coefficient
*/
float compute_G1(float alpha, float cost){
    float tant_square = 1/(cost*cost) - 1;
    float denom = 1 + alpha*alpha + tant_square;
    denom = 1 + sqrt(denom);
    return 2/denom;
}

/*
* this function calculates the specular component of the Cook-Torrance model for ligthing
*/
vec4 specular_cook(vec2 eta, float alpha, float cost, vec4 color, vec4 normal, vec4 light, vec4 eye, vec4 H, float I){
    float cosThetaI = dot(light,normal);
    float cosThetaO = dot(eye,normal);
    float F = fresnel_coeff(eta,cost);
    float D = compute_D(alpha,cost);
    float Gi = compute_G1(alpha,cosThetaI);
    float Go = compute_G1(alpha,cosThetaO);
    float coef = (F*D*Gi*Go) / (4*cosThetaI*cosThetaO);

    // float max = max_scalar_zero(normal,H);
    // max = pow(max,s);

    return coef * color * I;
    // return coef * color * max * I;
}

/*
main puts the good value in fragColor
*/
void main( void )
{
    vec4 vertNorm = normalize(vertNormal);
    // récupération de la couleur locale
    fragColor = vertColor;

    // récupération des variables :
    vec4 eyeVectorNorm = normalize(eyeVector);
    vec4 lightVectorNorm = normalize(lightVector);
    float ka = 0.5;
    float kd = 0.5;
    float alpha = 0.1;
    vec4 H = normalize(eyeVectorNorm +lightVectorNorm);
    float cos_theta_d = dot(H,lightVectorNorm);

    // Ambiant Lightning :
    vec4 Ca =ambiant(vertColor,ka,lightIntensity);


    // Diffuse Lightning
    vec4 Cd = diffuse(vertColor,vertNorm,lightVector,kd,lightIntensity);


    // Specular Lighting
    vec4 Cs = vec4(0,0,0,0);
    if (blinnPhong){
        // Blinn-Phong Model
        Cs  = specular_blinn(eta,cos_theta_d,vertColor,vertNorm,H,shininess,lightIntensity);
    }else{
        // Cook-Torrance Model
        Cs = specular_cook(eta,alpha,cos_theta_d,vertColor,vertNorm,lightVectorNorm,eyeVectorNorm,H,lightIntensity);
     }
     fragColor = Ca +Cd +Cs ;
}
