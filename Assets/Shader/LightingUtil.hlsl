//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float3 Diffuse;
    uint matpad0;
    float3 Specular;
    uint matpad1;
    float3 Ambient;
    uint matpad2;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // Linear falloff.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

float3 myBlinnPhongOfThreeParameters(Material mat,float3 LightStrength,float3 toeye,float3 lightVec,float3 normal)
{
    //L=La+Ld+Ls
   //KaIa+kd(I/r^2)max(0,n.dot(l))+ks(I/r^2)max(0,n.dot(h))^p

    float3 halfVect = normalize(toeye + lightVec);
    float hdotp= max(dot(normal, halfVect), 0.0f);
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 litcolor = (mat.Specular * LightStrength * hdotp) + (ndotl * LightStrength * mat.Diffuse) + mat.Ambient * LightStrength;
   // float3 litcolor = (ndotl * LightStrength * mat.Diffuse) + mat.Ambient * LightStrength;
    return litcolor;

}


float4 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    //L=La+Ld+Ls
    //KaIa+kd(I/r^2)max(0,n.dot(l))+ks(I/r^2)max(0,n.dot(h))^p
    //l
    float3 lightVec = L.Position - pos;

    float d = length(lightVec);
    //if (d > L.FalloffEnd)
   //  return float4(0.0f, 0.0f, 0.0f,0.0f);


float3 tempNormal=normal;
normal=normalize(tempNormal);

    //normalize 
   //lightVec /= d;

   lightVec=normalize(L.Position - pos);

    float3 lightStrength = L.Strength;
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength = L.Strength * att;

    return float4(myBlinnPhongOfThreeParameters(mat, lightStrength, toEye, normalize(lightVec), normalize(normal)), 1.0f);

    //return  float4(0.5f,1.0f,1.0f,1.0f);
    //return float4(lightStrength, 1.0f);
}




