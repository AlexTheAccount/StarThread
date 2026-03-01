cbuffer CameraBuffer : register(b0)
{
    matrix View;
    matrix Projection;
};

cbuffer ObjectBuffer : register(b1)
{
    matrix World;
}

struct PSIn
{
    float4 PosH : SV_POSITION;
    float2 UV : TEXCOORD0;
};

Texture2D diffTexture : register(t2);
SamplerState SampleType : register(s2);

float4 main(float4 pos : SV_POSITION, float2 texcoord : TEXCOORD0) : SV_TARGET
{
    float4 textureColor = diffTexture.Sample(SampleType, texcoord);
    return textureColor;
}