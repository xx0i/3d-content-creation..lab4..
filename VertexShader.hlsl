#pragma pack_matrix( row_major )   
// an ultra simple hlsl vertex shader
// TODO: Part 1f
struct VERTEX
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

// TODO: Part 2c // TODO: Part 4d
cbuffer VARIABLES
{
    matrix viewMatrix, perspectiveMatrix;
    vector lightColour, lightDir;
};
// TODO: Part 3b
struct OBJ_ATTRIB
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};


struct INSTANCE_DATA
{
    matrix worldMatrix;
    OBJ_ATTRIB material;
};

// TODO: Part 3c
StructuredBuffer<INSTANCE_DATA> drawInfo : register(b1, space0);

// TODO: Part 4a
struct OUTPUT_TO_RASTERIZER
{
    float4 posH : SV_POSITION;
    float3 posW : WORLD;
    float3 normW : NORMAL;
    float2 coordUV : TEXCOORD;
    nointerpolation uint index : Index;
};
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
struct OUTPUT
{
    float4 position : SV_POSITION;
    nointerpolation uint index : Index;
};

OUTPUT_TO_RASTERIZER main(VERTEX input, uint instanceID : SV_InstanceID) : SV_POSITION
{
	// TODO: Part 1h
	// TODO: Part 3g
    matrix worldMatrix = drawInfo[instanceID].worldMatrix;
    float4 worldPos = mul(float4(input.pos.xyz, 1), worldMatrix);
    // TODO: Part 2f
    float4 viewPos = mul(worldPos, viewMatrix);
    float4 perspectivePos = mul(viewPos, perspectiveMatrix);
	// TODO: Part 4b

    float3 worldNorm = normalize(mul(input.normal, (float3x3)worldMatrix)); //added in 4b + changed to new output

    OUTPUT_TO_RASTERIZER output;
    output.posH = perspectivePos;
    output.posW = worldPos;
    output.normW = worldNorm;
    output.index = instanceID;
    return output;
}