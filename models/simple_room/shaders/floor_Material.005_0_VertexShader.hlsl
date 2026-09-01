struct VS_INPUT
{
    float3 normal : NORMAL;
    float3 pos : POSITION;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 uv : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

cbuffer vpMatrixBufferContainer : register(b0)
{
    row_major float4x4 vpMatrixBuffer;
};

cbuffer MeshPrimitiveModelSpaceTransformBufferContainer : register(b1)
{
    row_major float4x4 meshPrimitiveModelSpaceTransformBuffer;
};

struct WorldRootTransformBuffersAllInstancesContainer // made a struct to accomodate row_major pattern
{
    row_major float4x4 buffer;
};

StructuredBuffer<WorldRootTransformBuffersAllInstancesContainer> worldRootTransformBuffersAllInstancesContainer : register(t0); // buffer srv


VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    float4x4 worldRootTransformForInstance = worldRootTransformBuffersAllInstancesContainer[input.instanceID].buffer;
    float4x4 modelToWorldMatrix = mul(meshPrimitiveModelSpaceTransformBuffer, worldRootTransformForInstance);
    float4x4 wvp = mul(modelToWorldMatrix, vpMatrixBuffer);
    
    output.pos = mul(float4(input.pos, 1.0f), wvp);
    output.uv = input.uv;
    output.uv1 = input.uv1;
    output.normal = input.normal;
    output.tangent = input.tangent;
    return output;
}