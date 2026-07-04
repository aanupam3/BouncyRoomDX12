struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;

};

/*static const float r = float(1.0f);
cbuffer BaseTrigTheta : register(b0)
{
    float sinBaseTheta;
    float cosBaseTheta;
}*/

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    // sin(A+B) = sinAcosB + cosAsinB;
    // sin(baseTheta + angleOffset)
    //float sinTheta = sinBaseTheta * input.cosOffset + cosBaseTheta * input.sinOffset;
    
    // cos(A+B) = cosAcosB - sinAsinB;
    // cos(baseTheta + angleOffset)
    //float cosTheta = cosBaseTheta * input.cosOffset - sinBaseTheta * input.sinOffset;
    
    //output.pos = float4(r * sinTheta, r * cosTheta, input.pos.z, 1.0f);
    
    output.pos = float4(input.pos, 1.0f);
    output.color = float4(input.color);
    output.uv = input.uv;
    return output;
}