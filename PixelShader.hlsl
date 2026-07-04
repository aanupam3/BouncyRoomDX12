struct PS_INPUT
{
	float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PS_OUTPUT
{
	float4 target : SV_TARGET;
};

Texture2D crateTexture : t0;
SamplerState crateTextureSampler : s0;

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    //output.target = float4(1, 1, 1, 1);
    output.target = crateTexture.Sample(crateTextureSampler, input.uv);
    return output;
}