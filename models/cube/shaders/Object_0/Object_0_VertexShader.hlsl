struct VS_INPUT
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    uint instanceID : SV_InstanceID;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
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
    return output;
}