// an ultra simple hlsl fragment shader
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

// TODO: Part 4a (optional)
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
float4 main() : SV_TARGET
{
	// TODO: Part 3e
	// TODO: Part 3h
    return float4(0.62f, 0.50f, 0.50f, 0); // TODO: Part 1a (optional)
	// TODO: Part 4c
	// TODO: Part 4d (half-vector or reflect method, your choice)
}