struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float2 uv2 : TEXCOORD2;
};

struct PS_OUTPUT
{
    float4 target : SV_TARGET;
};

Texture2D tex0 : register(t1);
Texture2D tex1 : register(t2);
Texture2D tex2 : register(t3);
Texture2D texBase : register(t5);
SamplerState textureSampler : s0;

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    //output.target = float4(1, 1, 1, 1);
    float4 sample0 = tex0.Sample(textureSampler, input.uv0);
    float4 sample1 = tex1.Sample(textureSampler, input.uv1);
    float4 sample2 = tex2.Sample(textureSampler, input.uv2);
    float4 sampleBase = texBase.Sample(textureSampler, input.uv0);
    output.target = sampleBase;
    return output;
}