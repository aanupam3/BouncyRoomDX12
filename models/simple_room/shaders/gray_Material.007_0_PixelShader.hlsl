struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
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
cbuffer lightsAndCameraContainer : register(b3)
{
    float3 lightDirection;
};

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
    float4 baseColor = colorFactors.baseColorFactor;
    float3 netNormal = input.normal;
    float4 colorWithLighting = mul(1 + (dot(normalize(lightDirection), normalize(netNormal))), baseColor);
    output.target = colorWithLighting;
    return output;
}