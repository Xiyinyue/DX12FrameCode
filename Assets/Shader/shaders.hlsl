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
Texture2D GBuffer[5]:register(t1);




VertexOut VSMain(VSinput input)
{
    VertexOut result;
    float4 position = float4(input.PosL, 1.0f);
    result.PosH = mul(position, MVP);
    result.PosW = mul(input.PosL, (float3x3)gWorld);
    result.NormalW = mul(input.NormalL,(float3x3)gWorld);
    result.TexC = input.TexC;
    return result;
}

float4 PSMain(VertexOut input) : SV_TARGET
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
    float3 toEye = normalize(viewPos - input.PosW);
    float4 a = ComputePointLight(Lights[0], mat, input.PosW, input.NormalW, toEye);
    // float4 a=float4(Lights[0].Strength,1.0f);
    float gamma = 0.5;
    a.rgb = pow(a.rgb, float(1 / gamma));
    a.a = 1;
    return a;
}