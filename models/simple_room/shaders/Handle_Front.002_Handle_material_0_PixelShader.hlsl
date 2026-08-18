struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

struct PS_OUTPUT
{
    float4 target : SV_TARGET;
};


PS_OUTPUT main(PS_INPUT input)
{
    PS_OUTPUT output;
    output.target = float4(1, 1, 1, 1);
    return output;
}