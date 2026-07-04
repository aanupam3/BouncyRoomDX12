struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
};

struct PS_OUTPUT
{
    float4 target : SV_TARGET;
};

Texture2D baseTexture : register(t1);
SamplerState textureSampler : s0;

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    //output.target = float4(1, 1, 1, 1);
    output.target = baseTexture.Sample(textureSampler, input.uv);
    return output;
}