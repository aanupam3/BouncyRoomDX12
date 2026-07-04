struct PS_INPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct PS_OUTPUT
{
    float4 target : SV_TARGET;
};

Texture2D myTexture : register(t0);
SamplerState textureSamplerState : register(s0);

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    // output.target = input.color;
    output.target = myTexture.Sample(textureSamplerState, input.uv);
    return output;
}