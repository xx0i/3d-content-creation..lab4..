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
// TODO: Part 3c
// TODO: Part 4a
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
float4 main(VERTEX input: POSITION) : SV_POSITION
{
	// TODO: Part 1h
	// TODO: Part 3g
    float3 tempShift = input.pos;
    tempShift.z += 0.75f;
    tempShift.y -= 0.75f;
	// TODO: Part 2f
	// TODO: Part 3h
	// TODO: Part 4b
    return float4(tempShift.xyz, 1);
}