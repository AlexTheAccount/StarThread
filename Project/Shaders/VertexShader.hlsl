cbuffer VP : register(b0)
{
    matrix View;
    matrix Projection;
}

cbuffer PushBlock : register(b1)
{
    matrix World;
};

struct VOut
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD;
};

VOut main(float4 position : POSITION, float2 texcoord : TEXCOORD)
{
    VOut output;
    
    float4 worldPosition = mul(position, World);
    float4 viewPosition = mul(worldPosition, View);
    output.position = mul(viewPosition, Projection);
    output.texcoord = texcoord;

    return output;
}