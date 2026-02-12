cbuffer Transform : register(b0)
{
    float4x4 World;
    float4x4 View;
    float4x4 Projection;
};

struct PSIn
{
    float4 PosH : SV_POSITION;
    float2 UV : TEXCOORD0;
    float3 Normal : TEXCOORD1;
};

Texture2D diffTexture : register(t2);
SamplerState SampleType : register(s2);

float4 main(float4 pos : SV_POSITION, float2 texcoord : TEXCOORD) : SV_TARGET
{
    float4 textureColor = diffTexture.Sample(SampleType, texcoord);
    return textureColor;
}