// an ultra simple hlsl fragment shader
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

// TODO: Part 4a (optional)
// TODO: Part 4b
// TODO: Part 3g
// TODO: Part 3h
float4 main() : SV_TARGET
{
	// TODO: Part 3e
    float4 diffuseColour = float4(drawInfo[1].material.Kd, 1);
    return diffuseColour;
	// TODO: Part 3h
    //return float4(0.62f, 0.50f, 0.50f, 0); // TODO: Part 1a (optional)
	// TODO: Part 4c
	// TODO: Part 4d (half-vector or reflect method, your choice)
    }