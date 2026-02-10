cbuffer Transform : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VOut
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : TEXCOORD1;
};

VOut main(float3 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    VOut output;

    float4 worldPosition = mul(float4(position, 1.0f), World);
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    output.texcoord = texcoord;
    output.normal = normal;

    return output;
}