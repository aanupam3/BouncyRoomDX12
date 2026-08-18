struct VS_INPUT
{
    float3 normal : NORMAL;
    float3 pos : POSITION;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

cbuffer wvpBuffer : register(b0)
{
    row_major float4x4 wvp;
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    return output;
}