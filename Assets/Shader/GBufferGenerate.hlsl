// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

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
    //float4x4 
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
struct MaterialData
{
    float3 Diffuse;
    uint matpad0;
    float3 Specular;
    uint matpad1;
    float3 Ambient;
    uint matpad2;
    // Used in texture mapping.
    float4x4 MatTransform;
};
StructuredBuffer<MaterialData> gMaterialData:register(t0);


SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);
SamplerComparisonState gsamShadow : register(s6);

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


VertexOut VS(VSinput input)
{
    VertexOut result;
    float4 position = float4(input.PosL, 1.0f);
    result.PosH = mul(position, MVP);
    result.PosW = mul(input.PosL, (float3x3)gWorld);
 
    result.NormalW = mul(input.NormalL, (float3x3)gWorld);
    result.TexC = input.TexC;
    return result;
}


void PS(VertexOut pin,
    out float4 Normal:SV_TARGET0,
    out float4 Position : SV_TARGET1,
    out float4 Diffuse : SV_TARGET2,
    out float4 Specular : SV_TARGET3,
    out float4 Ambient : SV_TARGET4)
{
    MaterialData matData = gMaterialData[MaterialIndex];
    // float4 a = float4(matData.Diffuse,1.0f);
    Material mat = {
        matData.Diffuse,
        matData.matpad0,
        matData.Specular,
        matData.matpad1,
        matData.Ambient,
        matData.matpad2
    };

    Normal = float4(normalize(pin.NormalW),1.0f);
    Position = float4(pin.PosW,1.0f);
    Diffuse = float4(matData.Diffuse, 1.0f);
    Specular= float4(matData.Specular, 1.0f);
    Ambient= float4(matData.Ambient, 1.0f);


}