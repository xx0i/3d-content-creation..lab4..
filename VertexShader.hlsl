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
struct OBJ_MATERIAL
{
    float4 data1; // 16 bytes
    float4 data2; // 16 bytes
    float4 data3; // 16 bytes
    float4 data4; // 16 bytes
    float4 data5; // 16 bytes
    float4 data6; // 16 bytes
    float4 data7; // 16 bytes
    float4 data8; // 16 bytes
    float4 data9; // 16 bytes
    float4 data10; // 16 bytes
    float4 data11; // 16 bytes -> total 176
};

struct INSTANCE_DATA
{
    matrix worldMatrix;
    OBJ_MATERIAL material;
};

// TODO: Part 3c
StructuredBuffer<INSTANCE_DATA> drawInfo : register(b1, space0);

// TODO: Part 4a
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
float4 main(VERTEX input: POSITION) : SV_POSITION
{
	// TODO: Part 1h
	// TODO: Part 3g
    //float3 tempShift = input.pos;
    //tempShift.z += 0.75f;
    //tempShift.y -= 0.75f;
	// TODO: Part 2f
    float4 result = mul(float4(input.pos.xyz, 1), viewMatrix);
    float4 pos = mul(result, perspectiveMatrix);
	// TODO: Part 3h
	// TODO: Part 4b
    return pos;
}