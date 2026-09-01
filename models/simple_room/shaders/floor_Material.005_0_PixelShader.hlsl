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

// 	t1: NORMAL,
//	t2: OCCLUSION,
//	t3: EMISSIVE,
//	t4: METALLIC_ROUGHNESS,
//	t5: PBR_BASE
cbuffer lightDirectionContainer : register(b3)
{
    float3 lightDirection;
};
Texture2D normalMap : register(t1);
Texture2D metallicaRoughnessMap : register(t4);
Texture2D baseTexture : register(t5);
SamplerState textureSampler : s0;

struct ColorFactors
{
    float4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
};

cbuffer colorFactorsContainer : register(b2)
{
    ColorFactors colorFactors;
};

PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    //output.target = float4(1, 1, 1, 1);
    float4 baseColor = colorFactors.baseColorFactor * baseTexture.Sample(textureSampler, input.uv);
    float3 netNormal = input.normal + normalMap.Sample(textureSampler, input.uv).xyz;
    float4 colorWithLighting = mul(1 + (dot(normalize(lightDirection), normalize(netNormal))), baseColor);
    output.target = colorWithLighting;
    return output;
}