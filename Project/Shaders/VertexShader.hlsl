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

VOut main(float4 position : POSITION, float2 texcoord : TEXCOORD, float3 normal : NORMAL)
{
    VOut output;

    float4 worldPosition = mul(position, World);
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    output.texcoord = texcoord;
    output.normal = normal;

    return output;
}