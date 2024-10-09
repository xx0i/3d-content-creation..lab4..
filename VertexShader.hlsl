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
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
struct OUTPUT
{
    float4 position : SV_POSITION;
    nointerpolation uint index : Index;
};

float4 main(VERTEX input, uint instanceID : SV_InstanceID) : SV_POSITION
{
	// TODO: Part 1h
    //float3 tempShift = input.pos;
    //tempShift.z += 0.75f;
    //tempShift.y -= 0.75f;
	// TODO: Part 3g
    matrix worldMatrix = drawInfo[instanceID].worldMatrix;
    float4 result = mul(float4(input.pos.xyz, 1), worldMatrix);
    // TODO: Part 2f
    result = mul(result, viewMatrix);
    float4 pos = mul(result, perspectiveMatrix);
	// TODO: Part 3h
    OUTPUT output;
    output.position = pos;
    output.index = instanceID;
	// TODO: Part 4b
    return output;
}