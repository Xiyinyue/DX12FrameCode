
#include "LightingUtil.hlsl"
cbuffer SceneConstantBuffer : register(b0)
{
    float4x4 MVP;
    float4x4 gWorld;
    float4 viewPos;
    float4 Look;
    float4 Up;
    float4 Right;
    float4 AmbientLight;
    float4x4 LightSpaceMatrix;
    Light Lights[MaxLights];
}
SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

struct VerttexIn
{
    float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC    : TEXCOORD;
};


struct VertexOut
{
    float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
};

Texture2D GBuffer[5]:register(t1);
Texture2D ShadowMap : register(t7);


float CalculateShadow(float3 shadowcoord)
{
   //float result = ShadowMap.Sample(gsamPointWrap,shadowcoord.xy);
    float result = ShadowMap.SampleCmpLevelZero(gsamShadow, shadowcoord.xy, (shadowcoord.z));
    return result;
    
}










VertexOut VS(VerttexIn input)
{
    VertexOut vout = (VertexOut)0.0f;

    // Already in homogeneous clip space.
    vout.PosH = float4(input.PosL, 1.0f);
	
	vout.TexC = input.TexC;
	
    return vout;
}











float4 PS(VertexOut pin): SV_TARGET
{
   
        float4 temp = GBuffer[2].Sample(gsamPointWrap, pin.TexC);
        float3 Diffuse = temp.rgb;
    
        float4 tempNormal = GBuffer[0].Sample(gsamPointWrap, pin.TexC);
        float3 Normal = tempNormal.rgb;

        float4 tempPos = GBuffer[1].Sample(gsamPointWrap, pin.TexC);
        float3 Position = tempPos.rgb;

        float4 tempSpecular = GBuffer[3].Sample(gsamPointWrap, pin.TexC);
        float3 Specular = tempSpecular.rgb;
     
        float4 tempAmbient = GBuffer[4].Sample(gsamPointWrap, pin.TexC);
        float3 Ambient = tempAmbient.rgb;
        Material mat = { Diffuse, 1, Specular, 1, Ambient, 1 };
        
       float shadow = 1.0f;
            float4 lightSpacePos = mul( float4(Position, 1.0f),LightSpaceMatrix);
            float4 shadowcoord = lightSpacePos / lightSpacePos.w;
            shadowcoord.rg = shadowcoord.rg * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
  
            shadow = CalculateShadow(shadowcoord.rgb);
    
        float3 toEye = normalize(viewPos - Position);
        float4 a = ComputePointLight(Lights[0], mat, Position, Normal, toEye, shadow);

            a.w = 1;
        
    
    
    
    
    
    
    
    
        float gamma = 0.5;
        a.rgb = pow(a.rgb, float(1 / gamma));
        a.a = 1;
        float3 lightVec = Lights[0].Position - Position;
        return a;
   // return float4(lightVec,1.0f);
    
   // float Debugresult = ShadowMap.Sample(gsamPointWrap, pin.TexC);
   // return float4(Debugresult, Debugresult, Debugresult, 1);
    
    }