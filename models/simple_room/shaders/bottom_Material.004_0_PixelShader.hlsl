struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
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

struct LightsAndCamera
{
    float4 lightPosition;
    float4 cameraPosition;
};
cbuffer lightsAndCameraContainer : register(b3)
{
    LightsAndCamera lightsAndCamera;
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
    float3 lightPos = lightsAndCamera.lightPosition;
    float3 cameraPos = lightsAndCamera.cameraPosition;
    float3 worldPos = input.worldPos;
    float4 baseColor = colorFactors.baseColorFactor * baseTexture.Sample(textureSampler, input.uv);
    float4 unlitColor = 0; // 0.25*baseColor;
   
    //float3 netNormal = input.normal + normalMap.Sample(textureSampler, input.uv).xyz;
    
    float3 l_norm = normalize(lightPos - worldPos);
    float3 n_norm = -normalize(input.normal);
    float3 r_norm = normalize(reflect(l_norm, n_norm));
    float3 v_norm = normalize(cameraPos - worldPos);
    
    float diffuse = (dot(n_norm, l_norm));
    float specular = 0; //(pow((dot(r_norm, v_norm)), 8.0));

    output.target = unlitColor + baseColor * diffuse + specular;

    return output;
}