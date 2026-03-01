// Camera matrices (binding b0)
cbuffer CameraBuffer : register(b0)
{
    matrix View;
    matrix Projection;
};

// Object/world matrix (binding b1)
cbuffer ObjectBuffer : register(b1)
{
    matrix World;
}

struct VOut
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD0;
};

VOut main(float3 inPos : POSITION, float2 inTex : TEXCOORD0)
{
    VOut output;
    
    float4 worldPos = mul(World, float4(inPos, 1.0f));
    float4 viewPos = mul(View, worldPos);
    float4 clipPos = mul(Projection, viewPos);

    output.pos = clipPos;
    output.texcoord = inTex;

    return output;
}