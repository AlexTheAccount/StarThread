cbuffer Transform : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer PushConstants
{
    matrix Model;
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
    
    float4 worldPosition = mul(Model, float4(position, 1.0f));
    float4 viewPosition  = mul(View,  worldPosition);
    output.position      = mul(Projection, viewPosition);

    output.texcoord = texcoord;
    output.normal = normal;

    return output;
}