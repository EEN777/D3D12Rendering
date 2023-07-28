#include "Common.hlsl"

StructuredBuffer<MaterialProperties> materialProperties[] : register(t0, space4);

// #DXR Extra - Another ray type
// Ray payload for the shadow rays
struct ShadowHitInfo
{
    bool isHit;
};

//struct Attributes
//{
//    float2 uv;
//};

[shader("closesthit")]
void ShadowClosestHit(inout ShadowHitInfo hit, Attributes bary)
{
    MaterialProperties currentMaterialProperties = materialProperties[InstanceID()].Load(0);
    
    if (currentMaterialProperties.isLight == true)
    {
        hit.isHit = false;
    }
    else
    {
        hit.isHit = true;
    }
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo hit : SV_RayPayload)
{
    hit.isHit = false;
}