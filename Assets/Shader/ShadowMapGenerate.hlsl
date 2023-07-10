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

cbuffer ObjConstantBuffer:register(b1)
{
    float4x4 WorldTransform;
    float4x4 TexTransform;
    uint MaterialIndex;
    uint     CBPad1;
    uint     CBPad2;
    uint     CBPad3;

}




struct VSinput 
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

float4 VSOnlyMain(VSinput input):SV_Position
{
    float4 result;
 
    //result = mul(float4(input.PosL.xyz, 1),WorldTransform );
   // result = mul(result, LightSpaceMatrix);
    result = mul(float4(input.PosL.xyz, 1), LightSpaceMatrix);
   // result.z *= result.w;  
    return result;
    
   // float4 debr;  
    //debr = mul(float4(input.PosL.xyz, 1), MVP);
   // return debr;
      
}
