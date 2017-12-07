#version 410
#define M_PI 3.14159265358979323846

uniform mat4 mat_inverse;
uniform mat4 persp_inverse;
uniform sampler2D envMap;
uniform vec3 center;
uniform float radius;

uniform bool transparent;
uniform float shininess;
uniform bool bubble;
uniform vec2 eta;
uniform int refractions;
uniform float innerRadius;
in vec4 position;

out vec4 fragColor;

/*
* This function returns the color of the environmentMap in the direction given in parameter
*/
vec4 getColorFromEnvironment(in vec3 direction)
{
    float phi = acos(-direction.y);
    phi = (phi) / (M_PI);
    float theta = atan(direction.z,direction.x);
    theta = (theta + M_PI) / (2*M_PI);
    return texture2D(envMap,vec2(theta,phi));
}

/*
* This function calculates the Fresnel coefficient
*/
float fresnel_coeff(float eta, float cost){
  float ci = abs(eta*eta + 1 - cost*cost) ;
  ci = sqrt(ci);
  float nume = cost - ci;
  float denom = cost + ci;
  float Fs = abs(nume/denom);
  Fs = Fs*Fs;
  nume = eta*eta*cost - ci;
  denom = eta*eta*cost + ci;
  float Fp = abs(nume/denom);
  Fp = Fp * Fp;
  return((Fs+Fp)/2);
  }

bool raySphereIntersect(in vec3 start, in vec3 direction,in float radius, out vec3 newPoint) {
    vec3 pc = center - start;
    float r = radius ;
    float pcd = dot(pc, direction);
    float Delta = pcd * pcd - dot(pc, pc) + r * r;
    float delta;
    if (Delta > 0){
        delta = sqrt(Delta);
        if (pcd - delta > 0){
            newPoint = start + (pcd - delta) * direction;
            return true;
        } else if (pcd + delta > 0){
            newPoint = start + (pcd + delta) * direction;
            return true;
        }
    }
    return false;
}

/*
* this function compute the normal, the reflected and the refracted ray when the normal is
* toward the outside of the bubbles
*/
void computeRays(in vec3 pointInter,in out vec3 normal,in out vec3 reflected,in out vec3 refracted,in vec3 direction){
  normal = normalize(center-pointInter);
  reflected = reflect(direction,normal);
  reflected= normalize(reflected);
  refracted = refract (direction,normal,eta.x);
  refracted = normalize(refracted);
}

/*
* this function compute the normal, the reflected and the refracted ray when the normal is
* toward the inside of the bubbles
*/
void computeRaysBubble(in vec3 pointInter,in out vec3 normal,in out vec3 reflected,in out vec3 refracted,in vec3 direction){
  normal = normalize(pointInter-center);
  reflected = reflect(direction,normal);
  reflected= normalize(reflected);
  refracted = refract (direction,normal,eta.x);
  refracted = normalize(refracted);
}

/*
* This function returns the fresnel coefficient
*/
float computeFresnel(in vec3 reflected, in vec3 normal,in float eta){
  float cos_theta_d = dot(reflected,normal);
  float F = fresnel_coeff(eta,cos_theta_d);
  //if(cos_theta_d<0) F = 1 ;
  //if (F>1)F=1;
  return F ;
}

/*
* this function computes the color of thez pixel when it's inside a single sphere
*/
void computeRefractions(in vec3 pointInter,in vec3 normal,in vec3 reflected,in out vec3 refracted){
  float F = computeFresnel(reflected,normal,eta.x);
  float coeff = (1-F);
  fragColor = F *getColorFromEnvironment(reflected);
  vec3 direction = normalize(refracted);
  // Boucle pour répéter les réfractions
  for(int i = 0 ; i < refractions ; i ++){
    pointInter += 0.001*direction ;
    bool inter = raySphereIntersect(pointInter,direction,radius,pointInter);
    computeRays(pointInter,normal,reflected,refracted,direction);
    F =computeFresnel(reflected,normal,1/eta.x);
    if(getColorFromEnvironment(refracted).x>0){
    fragColor += coeff*(1-F)*getColorFromEnvironment(refracted);}
    coeff *= F ;
    direction = reflected ;
  }
}

/*
* This function computes the refractions of the rays between the two spheres when it's already been inside the two.
*/
void computeExtBubbles(in vec3 pointInter,in vec3 normal,in vec3 reflected,in vec3 refracted,in float coeff, in int ite){
  // Interface air->verre
  float F = computeFresnel(reflected,normal,eta.x);
  coeff = coeff*(1-F);
  vec3 direction = refracted;
    pointInter += 0.001*direction ;
    // Point de type P4
    bool inter = raySphereIntersect(pointInter,direction,radius,pointInter);
    computeRays(pointInter,normal,reflected,refracted,direction);
    // Calcul de Frenel avec interface verre-> air
    F =computeFresnel(reflected,normal,1/eta.x);
    // ##################    SORTIE RAYON 5    ###################
    if(getColorFromEnvironment(refracted).x>0){
      fragColor += coeff*(1-F)*getColorFromEnvironment(refracted);
    }
}

// void computeExtBubbles(in vec3 pointInter,in vec3 normal,in vec3 reflected,in vec3 refracted,in float coeff,in int ite){
//   bool toBubbleInt = false;
//   bool inter;
//   // Interface air->verre
//   float F = computeFresnel(reflected,normal,eta);
//   coeff = coeff*(1-F);
//   vec3 direction = refracted;
//   for(int i = ite; i < refractions+1 ; i++){
//     pointInter += 0.001*direction ;
//     if(toBubbleInt){
//       inter = raySphereIntersect(pointInter,direction,radius*innerRadius,pointInter);
//       if(!(inter)){
//         inter = raySphereIntersect(pointInter,direction,radius,pointInter);
//         toBubbleInt=false;
//       }
//       }else{
//         inter=raySphereIntersect(pointInter,direction,radius,pointInter);
//       }
//       // Point de type P4
//       bool inter = raySphereIntersect(pointInter,direction,radius,pointInter);
//       if(toBubbleInt){
//         // Point type P3'
//         computeRaysBubble(pointInter,normal,reflected,refracted,direction);
//       }else{
//         // Point type P4
//         computeRays(pointInter,normal,reflected,refracted,direction);
//       }
//       // Calcul de Frenel avec interface verre-> air
//       F =computeFresnel(reflected,normal,1/eta);
//       // ##################    SORTIE RAYON 5    ###################
//       if(!(toBubbleInt)){
//         if(getColorFromEnvironment(refracted).x>0){
//           fragColor += coeff*(1-F)*getColorFromEnvironment(refracted);
//         }
//         break;
//       }else{
//        // #########  Calcul rayon 4 #####
//        coeff *= F ;
//        direction = reflected ;
//        toBubbleInt = !(toBubbleInt);
//      }
//    }
// }

/*
* This function computes the refractions of the ray inside the second sphere of the bubble
*/
void computeIntBubbles(in vec3 pointInter,in vec3 normal,in vec3 reflected,in vec3 refracted,in float coeff,in int ite){
  bool inter ;
  vec3 direction = refracted;
  float F ;
  for(int i = ite; i < refractions+1 ; i++){
    pointInter += 0.001*direction ;
    inter = raySphereIntersect(pointInter,direction,radius*innerRadius,pointInter);
    computeRays(pointInter,normal,reflected,refracted,direction);
    // interface air-> verre
    refracted=refract(direction,normal,1/eta.x);
    F =computeFresnel(reflected,normal,eta.x);
    // ###########    Vers le calcul du RAYON 4 #########
    computeExtBubbles(pointInter,normal,reflected,refracted,coeff*(1-F),i);
    coeff *= F ;
    // ########  On continue le RAYON 3  #######
    direction = reflected ;
  }
}

/*
* this function computes the refractions of the rays when they enter the bubbles
*/
void computeBubbles(in vec3 pointInter,in vec3 normal,in vec3 reflected,in vec3 refracted,in float coeff,in int ite){
  // Direction du vecteur
  bool toBubbleInt = true;
  // Interface air->Verre
  float F = computeFresnel(reflected,normal,eta.x);
  coeff = coeff*(1-F);
  bool inter ;
  vec3 direction = refracted;
  // ##############          SORTIE RAYON 1         ##########
  fragColor = F *getColorFromEnvironment(reflected);
  //      Boucle pour repeter les refractions entre la première et la seconde bulle
  for(int i = ite; i < refractions+1 ; i++){  // tant qu'on a encore des réfractions possibles
    pointInter += 0.001*direction ;
    if(toBubbleInt){ // Si le rayon se dirige vers l'intérieur de la bulle ext.
      // Point de type P2
       inter = raySphereIntersect(pointInter,direction,radius*innerRadius,pointInter);
       // Si le rayon est en dehors de la bulle int, on est encore dans la bulle ext !
       if(!(inter)){
         inter = raySphereIntersect(pointInter,direction,radius,pointInter);
         toBubbleInt = false;
       }
    }else {
      // Point de type P1
       inter = raySphereIntersect(pointInter,direction,radius,pointInter);
    }
    // On a maintenant le bon point d'intersection.
    if(toBubbleInt){ // Si on touche la bulle intérieure
      // Point type P2
      computeRaysBubble(pointInter,normal,reflected,refracted,direction); // calcul du point d'intersection avec la normale bien orientée
    }else{
      // Point type P1
      computeRays(pointInter,normal,reflected,refracted,direction); // calcul du point d'inter avec la norm. bien orientée.
    }
    // Calcul de Frenel avec interface verre-> air
    F =computeFresnel(reflected,normal,1/eta.x);
    // On ne fait ressortir que si on est vers la bulle ext.
    if(!(toBubbleInt)){
      // ##################    SORTIE RAYON 2     ###################
      if(getColorFromEnvironment(refracted).x>0){
        fragColor += coeff*(1-F)*getColorFromEnvironment(refracted);
      }
      break;
    }else {
      // ################# Vers le calcul du RAYON 3  ##################" "
      computeIntBubbles(pointInter,normal,reflected,refracted,coeff*(1-F),i);
     }

    // ###### On continu le calcul pour les rayons 2  #####
    coeff *= F ;
    direction = reflected ;
    toBubbleInt = !(toBubbleInt);
  }
}


/*
* main puts the good value in fragcolor
*/
void main(void)
{
    // Step 1: I need pixel coordinates. Division by w?
    vec4 worldPos = position;
    worldPos.z = 1; // near clipping plane
    worldPos = persp_inverse * worldPos;
    worldPos /= worldPos.w;
    worldPos.w = 0;
    worldPos = normalize(worldPos);
    // Step 2: ray direction:
    vec3 u = normalize((mat_inverse * worldPos).xyz);
    vec3 eye = (mat_inverse * vec4(0, 0, 0, 1)).xyz;


    vec4 resultColor = vec4(0,0,0,1);
    // Step 3: detect intersection
    vec3 pointInter ;
    bool inter = raySphereIntersect(eye,u,radius,pointInter);
    if(inter){
      vec3 normal = normalize(pointInter-center);
      vec3 reflected = reflect(u,normal);
      reflected= normalize(reflected);
      // Interface air -> verre
      vec3 refracted = refract (u,normal,1/eta.x);
      refracted = normalize(refracted);

      if(transparent){
        if(bubble){
          // Modèle Bulles :
          computeBubbles(pointInter,normal,reflected,refracted,1.0,0);
        }else{
          // Modèle Sphere :
          computeRefractions(pointInter,normal,reflected,refracted);
        }

      } else {
        fragColor = getColorFromEnvironment(reflected) ;
      }


    }else
      fragColor = getColorFromEnvironment(u);

}
