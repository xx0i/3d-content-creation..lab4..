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
struct OUTPUT_TO_RASTERIZER
{
    float4 posH : SV_POSITION;
    float3 posW : WORLD;
    float3 normW : NORMAL;
    float2 coordUV : TEXCOORD;
    nointerpolation uint index : Index;
};
// TODO: Part 3g
// TODO: Part 3h
struct OUTPUT
{
    float4 position : SV_POSITION;
    nointerpolation uint index : Index;
};
// TODO: Part 4b
float4 main(OUTPUT_TO_RASTERIZER input) : SV_TARGET
{
	// TODO: Part 3e
    float3 diffuseColour = drawInfo[input.index].material.Kd;
	// TODO: Part 3h
    //return float4(0.62f, 0.50f, 0.50f, 0); // TODO: Part 1a (optional)
	// TODO: Part 4c
    float3 norm = normalize(input.normW);
 //   float3 lightDirection = normalize(-lightDir);
    float diffuseIntensity = clamp(dot(norm, lightDirection), 0.0f, 1.0f);
    float3 finalColour = diffuseIntensity * diffuseColour * lightColour.xyz;
    
	// TODO: Part 4d (half-vector or reflect method, your choice)
    
    return float4(finalColour, lightColour.w);
}