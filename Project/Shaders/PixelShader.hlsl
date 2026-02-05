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

Texture2D diffTexture : register(t1);
SamplerState SampleType : register(s1);

float4 main(PSIn input) : SV_TARGET
{
    float3 normal = normalize(input.Normal);
    float4 color = diffTexture.Sample(SampleType, input.UV);
    float3 lightDirection = normalize(float3(0.5f, 0.8f, 0.5f));
    float lambert = max(dot(normal, lightDirection), 0.0f);
    color.rgb *= (0.2f + 0.8f * lambert);

    return color;
}