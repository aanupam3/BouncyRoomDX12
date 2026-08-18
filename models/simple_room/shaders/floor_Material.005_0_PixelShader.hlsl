struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct PS_OUTPUT
{
    float4 target : SV_TARGET;
};

Texture2D baseTexture : register(t0);
SamplerState textureSampler : s0;

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    output.target = float4(0.5, 0.5, 0.5, 1);
    //output.target = baseTexture.Sample(textureSampler, input.uv);
    return output;
}